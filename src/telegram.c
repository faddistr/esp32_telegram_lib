#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include "telegram_log.h"
#include "telegram_hal.h"
#include "telegram.h"
#include "telegram_io.h"
#include "telegram_getter.h"

#define TELEGRAM_DEBUG 0

static const char *TAG="telegram_core";

#define TELEGRAM_BOUNDARY_HDR "----------------------785aad86516cca68"
#define TELEGRAM_BOUNDARY "--"TELEGRAM_BOUNDARY_HDR
#define TELEGRAM_BOUNDARY_CONTENT_FMT TELEGRAM_BOUNDARY"\r\nContent-Disposition: form-data; name="
#define TELEGRAM_BOUNDARY_FTR "\r\n"TELEGRAM_BOUNDARY"--\r\n"

typedef struct 
{
  void *teleCtx;
  void *user_ctx;
  telegram_evt_cb_t user_cb;
  uint32_t total_len;
} telegram_send_data_e_t;

const telegram_io_header_t jsonHeaders[] = 
{
  {"Content-Type", "application/json"},
  {NULL, NULL}
};

typedef struct
{
  char *token;
  void *getter;
  void *io_ctx;
  telegram_on_msg_cb_t on_msg_cb;
  telegram_int_t last_update_id;
  uint32_t max_messages;
#ifdef ESP32
  TELEGRAM_SEMAPHORE_t sem;
#endif
} telegram_ctx_t;

#ifdef ESP32
#if TELGRAM_DEBUG == 1
#define telegram_wait_mutex(x) { \
  TELEGRAM_LOGI(TAG, "Taking mutex %s", __func__); \
  telegram_wait_mutex_func(&x->sem, (char *)__func__); \
  }
#define telegram_give_mutex(x) { \
  TELEGRAM_LOGI(TAG, "Give mutex %s", __func__); \
  telegram_give_mutex_func(&x->sem); \
  }

#else 
#define telegram_wait_mutex(x) telegram_wait_mutex_func(&((telegram_ctx_t *)x)->sem, (char *)__func__);
#define telegram_give_mutex(x) telegram_give_mutex_func(&((telegram_ctx_t *)x)->sem);
#endif
#else
//This was done to prevent overload of the device
#define telegram_wait_mutex(x)
#define telegram_give_mutex(x)
#endif


static void telegram_process_message_int_cb(void *hnd, telegram_update_t *upd)
{
  telegram_ctx_t *teleCtx = NULL;

  if ((hnd == NULL) || (upd == NULL))
    {
      TELEGRAM_LOGE(TAG, "Internal error during processing messages");
      return;
    }

  teleCtx = (telegram_ctx_t *)hnd;
  teleCtx->last_update_id = upd->id;
  teleCtx->on_msg_cb(teleCtx, upd);
}

static void telegram_getMessages(void *ctx)
{

#if TELEGRAM_LONG_POLLING == 1
  const telegram_io_header_t sendHeaders[] =
  {
    {"Prefer", "wait=120"},
    {NULL, NULL}
  };
#endif
  char *buffer = NULL;
  char *path = NULL;
  telegram_ctx_t *teleCtx = (telegram_ctx_t *)ctx;

  if (!ctx)
    {
      TELEGRAM_LOGE(TAG, "Internal error!");
      return;
    }

  telegram_wait_mutex(teleCtx);

  path = telegram_make_method_path(TELEGRAM_GET_UPDATES, teleCtx->token, teleCtx->max_messages,
                                   (teleCtx->last_update_id?(teleCtx->last_update_id + 1):0), NULL);

  if (!path)
    {
      TELEGRAM_LOGE(TAG, "No mem!");
      telegram_give_mutex(teleCtx);
      return;
    }

#if TELEGRAM_LONG_POLLING == 1
  buffer = telegram_io_get_ctx(&teleCtx->io_ctx, path, (telegram_io_header_t *)sendHeaders);
#else
  buffer = telegram_io_get_ctx(&teleCtx->io_ctx, path, NULL);
#endif
  free(path);
  if (buffer != NULL)
    {
      telegram_parse_messages(teleCtx, buffer, telegram_process_message_int_cb);
      free(buffer);
    }

  telegram_give_mutex(teleCtx);
}

void telegram_stop(void *teleCtx_ptr)
{
  telegram_ctx_t *teleCtx = (telegram_ctx_t *)teleCtx_ptr;

  if (teleCtx_ptr == NULL)
    {
      return;
    }

  telegram_getter_stop(teleCtx->getter);
  telegram_io_free_ctx(&teleCtx->io_ctx);
#ifdef ESP32
  TELEGRAM_DESTROY_SEMAPHORE(teleCtx->sem);
#endif
  free(teleCtx->token);
  free(teleCtx);
}

void *telegram_init(const char *token, uint32_t max_messages, telegram_on_msg_cb_t on_msg_cb)
{
  telegram_ctx_t *teleCtx = NULL;

  if (on_msg_cb == NULL)
    {
      return NULL;
    }

  teleCtx = calloc(1, sizeof(telegram_ctx_t));
  if (teleCtx != NULL)
    {
      teleCtx->max_messages = max_messages;

      if (teleCtx->max_messages == 0)
        {
          teleCtx->max_messages = TELEGRAM_DEFAULT_MESSAGE_LIMIT;
        }
#ifdef ESP32
      TELEGRAM_CREATE_SEMAPHORE(teleCtx->sem);
#endif
      teleCtx->token = strdup(token);
      teleCtx->on_msg_cb = on_msg_cb;
      teleCtx->getter = telegram_getter_init(telegram_getMessages, teleCtx);

      if (!teleCtx->getter)
        {
          TELEGRAM_LOGE(TAG, "Failed to init getter");
          free(teleCtx);
          return NULL;
        }
    }

  return teleCtx;
}

static void telegram_send_message(void *teleCtx_ptr, telegram_int_t chat_id, const char *message, 
                                  telegram_kbrd_t *kbrd)
{
  char *path = NULL;
  char *payload = NULL;
  telegram_ctx_t *teleCtx = (telegram_ctx_t *)teleCtx_ptr;
  if (!teleCtx)
    {
      return;
    }

  telegram_wait_mutex(teleCtx);

  path = telegram_make_method_path(TELEGRAM_SEND_MESSAGE, teleCtx->token, 0, 0, NULL);
  if (path == NULL)
    {
      telegram_give_mutex(teleCtx);
      return;
    }

  payload = telegram_make_message(chat_id, message, kbrd);
  if (payload == NULL)
    {
      free(path);
      telegram_give_mutex(teleCtx);
      return;
    }

  TELEGRAM_LOGI(TAG, "Send message: %s %s", path, payload);
  telegram_io_send(path, payload, (telegram_io_header_t *)jsonHeaders);
  free(path);
  free(payload);
  telegram_give_mutex(teleCtx);
}

void telegram_kbrd(void *teleCtx_ptr, telegram_int_t chat_id, const char *message, telegram_kbrd_t *kbrd)
{	
  telegram_send_message(teleCtx_ptr, chat_id, message, kbrd);
}

void telegram_send_text_message(void *teleCtx_ptr, telegram_int_t chat_id, const char *message)
{
  telegram_send_message(teleCtx_ptr, chat_id, message, NULL);
}

void telegram_send_text(void *teleCtx_ptr, telegram_int_t chat_id, telegram_kbrd_t *kbrd, const char *fmt, ...)
{
  char *str;
  size_t len;
  va_list ptr;

  va_start(ptr, fmt);
  len = vsnprintf(NULL, 0, fmt, ptr) + 1;
  str = malloc(len);

  if (str != NULL)
    {
      vsprintf(str, fmt, ptr);
      telegram_send_message(teleCtx_ptr, chat_id, str, kbrd);
      free(str);
    } else
    {
      TELEGRAM_LOGE(TAG, "No mem!");
    }

  va_end(ptr);
}

char *telegram_get_file_path(void *teleCtx_ptr, const char *file_id)
{
  char *buffer = NULL;
  char *ret = NULL;
  char *path = NULL;
  telegram_ctx_t *teleCtx = (telegram_ctx_t *)teleCtx_ptr;

  if (teleCtx == NULL)
    {
      TELEGRAM_LOGE(TAG, "Send message: Null argument");
      return NULL;
    }

  telegram_wait_mutex(teleCtx);
  path = telegram_make_method_path(TELEGRAM_GET_FILE_PATH, teleCtx->token, 0, 0, file_id);
  if (path == NULL)
    {
      telegram_give_mutex(teleCtx);
      return NULL;
    }

  TELEGRAM_LOGI(TAG, "Send getFile: %s", path);
  buffer = telegram_io_get(path, NULL);
  free(path);
  if (buffer != NULL)
  {
      char *file_path = telegram_parse_file_path(buffer);
      free(buffer);

      if (file_path != NULL)
        {
          ret = telegram_make_method_path(TELEGRAM_GET_FILE, teleCtx->token, 0, 0, file_path);
          free(file_path);
        }
  }
  telegram_give_mutex(teleCtx);
  return ret;
}

static uint32_t telegram_send_file_cb(void *ctx, uint8_t *buf, uint32_t max_size, uint32_t offset)
{
  uint32_t write_size = 0;
  telegram_send_data_e_t *hnd = (telegram_send_data_e_t *)ctx;
  telegram_write_data_evt_t evt = {.buf = buf, .pice_size = max_size, .offset = offset };

  evt.total_size = hnd->total_len;
  if (max_size > hnd->total_len)
    {
      evt.pice_size -= strlen(TELEGRAM_BOUNDARY_FTR);
    }

  write_size = hnd->user_cb(TELEGRAM_READ_DATA, hnd->teleCtx, hnd->user_ctx, &evt);
  if (write_size > evt.pice_size)
    {
      TELEGRAM_LOGE(TAG, "write_size > size_to_send");
      hnd->user_cb(TELEGRAM_ERR, hnd->teleCtx, hnd->user_ctx, NULL);
      return 0;
    }

  hnd->total_len -= write_size;
  if (evt.pice_size != max_size)
    {
      sprintf((char *)&buf[write_size], "%s", TELEGRAM_BOUNDARY_FTR);
      write_size += strlen(TELEGRAM_BOUNDARY_FTR);
    }

  return write_size;
}

static void parse_response_result(void *teleCtx, telegram_update_t *info)
{
  telegram_send_data_e_t *hnd = (telegram_send_data_e_t *)teleCtx;

  hnd->user_cb(TELEGRAM_RESPONSE, hnd->teleCtx, hnd->user_ctx, info);
}

static bool telegram_io_get_file_cb(void *ctx, uint8_t *buf, int size, int total_len)
{
  telegram_write_data_evt_t evt = {.buf = buf, .pice_size = (uint32_t)size,
                                   .total_size = (uint32_t)total_len};
  telegram_send_data_e_t *hnd = (telegram_send_data_e_t *)ctx;

  if ((size < 0) || (total_len == 0) || (buf == NULL))
    {
      hnd->user_cb(TELEGRAM_ERR, hnd->teleCtx, hnd->user_ctx, NULL);
      return false;
    }

  if (0 == hnd->user_cb(TELEGRAM_WRITE_DATA, hnd->teleCtx, hnd->user_ctx, &evt))
    {
      return false;
    }

  return true;
}

void telegram_send_file_full(void *teleCtx_ptr, telegram_int_t chat_id, char *caption, char *filename, uint32_t total_len,
                             void *ctx, telegram_evt_cb_t cb, telegram_file_type_t file_type)
{
  const telegram_io_header_t sendHeaders[] =
  {
    {"Content-Type", "multipart/form-data; boundary="TELEGRAM_BOUNDARY_HDR},
    {NULL, NULL}
  };

  char *path = NULL;
  char *overhead = NULL;
  char *response = NULL;
  telegram_ctx_t *teleCtx = (telegram_ctx_t *)teleCtx_ptr;
  telegram_send_data_e_t *ctx_e = NULL;

  if ((teleCtx_ptr == NULL) || (cb == NULL) || (total_len == 0) || (filename == NULL))
    {
      TELEGRAM_LOGE(TAG, "Send file: Wrong argument");
      return;
    }

  telegram_wait_mutex(teleCtx);
  switch(file_type)
    {
    case TELEGRAM_PHOTO:
      path = telegram_make_method_path(TELEGRAM_SEND_PHOTO, teleCtx->token, 0, 0, NULL);
      break;

    default:
      path = telegram_make_method_path(TELEGRAM_SEND_FILE, teleCtx->token, 0, 0, NULL);
      break;
    }

  if (path == NULL)
    {
      TELEGRAM_LOGE(TAG, "No mem (1)!");
      telegram_give_mutex(teleCtx);
      return;
    }

  overhead = calloc(sizeof(char), ((caption!=NULL)?strlen(caption):0) + strlen(filename) + TELEGRAM_INT_MAX_VAL_LENGTH
                    + 3 * strlen(TELEGRAM_BOUNDARY_CONTENT_FMT) + 2 * strlen(TELEGRAM_BOUNDARY"\r\n") + strlen(TELEGRAM_BOUNDARY_FTR));
  if (overhead == NULL)
    {
      TELEGRAM_LOGE(TAG, "No mem (2)!");
      free(path);
      telegram_give_mutex(teleCtx);
      return;
    }

  sprintf(overhead, TELEGRAM_BOUNDARY_CONTENT_FMT"\"chat_id\"\r\n\r\n%.0f\r\n", chat_id);
  if (caption)
    {
      sprintf(&overhead[strlen(overhead)], TELEGRAM_BOUNDARY_CONTENT_FMT"\"caption\"\r\n\r\n%s\r\n", caption);
    }


  switch(file_type)
    {
    case TELEGRAM_PHOTO:
      sprintf(&overhead[strlen(overhead)], TELEGRAM_BOUNDARY_CONTENT_FMT"\"photo\"");
      break;

    default:
      sprintf(&overhead[strlen(overhead)], TELEGRAM_BOUNDARY_CONTENT_FMT"\"document\"");
      break;
    }

  sprintf(&overhead[strlen(overhead)], "; filename=\"%s\"\r\nContent-Type: application/octet-stream\r\n\r\n", filename);

  ctx_e = calloc(1, sizeof(telegram_send_data_e_t));
  if (!ctx_e)
    {
      TELEGRAM_LOGE(TAG, "No mem!(3)");
      free(response);
      free(overhead);
      free(path);
      telegram_give_mutex(teleCtx);
      return;
    }

  ctx_e->teleCtx = teleCtx_ptr;
  ctx_e->user_ctx = ctx;
  ctx_e->user_cb = cb;
  ctx_e->total_len = total_len;

  total_len += strlen(TELEGRAM_BOUNDARY_FTR);
  response = telegram_io_send_big(path, total_len, (telegram_io_header_t *)sendHeaders, overhead,
                                  ctx_e, telegram_send_file_cb);

  free(overhead);
  free(path);
  if (response)
    {
      telegram_parse_messages(ctx_e, response, parse_response_result);
      free(response);
    }

  cb(TELEGRAM_END, ctx_e->teleCtx, ctx_e->user_ctx, NULL);
  free(ctx_e);
  telegram_give_mutex(teleCtx);
}

void telegram_send_file(void *teleCtx_ptr, telegram_int_t chat_id, char *caption, char *filename, uint32_t total_len,
                        void *ctx, telegram_evt_cb_t cb)
{
  telegram_send_file_full(teleCtx_ptr, chat_id, caption, filename, total_len, ctx, cb, TELEGRAM_DOCUMENT);
}


void telegram_get_file(void *teleCtx_ptr, const char *file_id, void *ctx, telegram_evt_cb_t cb)
{
  char *file_path = NULL;
  telegram_send_data_e_t ctx_e = {.teleCtx = teleCtx_ptr, .user_ctx = ctx, .user_cb = cb, };

  if ((teleCtx_ptr == NULL) || (cb == NULL))
    {
      TELEGRAM_LOGE(TAG, "NULL argument");
      return;
    }

  file_path = telegram_get_file_path(teleCtx_ptr, file_id);
  telegram_wait_mutex((telegram_ctx_t *)teleCtx_ptr);
  if (file_path == NULL)
    {
      telegram_give_mutex((telegram_ctx_t *)teleCtx_ptr);
      ctx_e.user_cb(TELEGRAM_ERR, ctx_e.teleCtx, ctx_e.user_ctx, NULL);
      TELEGRAM_LOGE(TAG, "Fail to get file path");
    } else
    {
      telegram_io_read_file(file_path, &ctx_e, telegram_io_get_file_cb);
      free(file_path);
      telegram_give_mutex((telegram_ctx_t *)teleCtx_ptr);
      ctx_e.user_cb(TELEGRAM_END, ctx_e.teleCtx, ctx_e.user_ctx, NULL);
    }
}

void telegram_answer_cb_query(void *teleCtx_ptr, const char *cid, const char *text, 
                              bool show_alert, const char *url, telegram_int_t cache_time)
{
  char *path = NULL;
  char *str = NULL;
  telegram_ctx_t *teleCtx = (telegram_ctx_t *)teleCtx_ptr;

  if ((teleCtx_ptr == NULL) || (cid == NULL))
    {
      TELEGRAM_LOGE(TAG, "NULL argument");
      return;
    }

  telegram_wait_mutex(teleCtx);
  str = telegram_make_answer_query(cid, text, show_alert, url, cache_time);
  if (str == NULL)
    {
      TELEGRAM_LOGE(TAG, "No memory!(1)");
      telegram_give_mutex(teleCtx);
      return;
    }

  path = telegram_make_method_path(TELEGRAM_ANSWER_QUERY, teleCtx->token, 0, 0, NULL);
  if (path == NULL)
    {
      free(str);
      TELEGRAM_LOGE(TAG, "No memory!(2)");
      telegram_give_mutex(teleCtx);
      return;
    }

  telegram_io_send(path, str, (telegram_io_header_t *)jsonHeaders);
  free(path);
  free(str);
  telegram_give_mutex(teleCtx);
}
