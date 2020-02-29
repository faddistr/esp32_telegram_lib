#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "telegram_log.h"
#include "telegram_io_hal.h"

#define TELEGRAM_IO_MAX_BUFF_SIZE 4096
#define MIN(x, y) (((x) < (y))?(x):(y))

static const char *TAG="telegram_io_hal";

typedef struct
{
  uint32_t total_len;
  uint32_t offset;
  telegram_io_send_file_cb_t cb;
  void *user_data;
} telegram_io_int_ctx_t;

typedef struct
{
  char *buffer;
  uint32_t size;
} telegram_io_mem_t;

static struct curl_slist *telegram_io_set_headers_platform(telegram_io_header_t *headers)
{
    char buffer[256];
    struct curl_slist *chunk = NULL;
    int i = 0;

    if (headers == NULL)
    {
      return NULL;
    }

    while (headers[i].key)
    {
        snprintf(buffer, sizeof(buffer), "%s: %s", headers[i].key, headers[i].value);
        chunk = curl_slist_append(chunk, buffer);
        i++;
    }

    return chunk;
}

static struct curl_slist *telegram_io_prepare_platform(CURL *curl, const char *path, bool is_post, telegram_io_header_t *headers)
{
  struct curl_slist *slist = telegram_io_set_headers_platform(headers);
  if (slist != NULL)
  {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
  }

  if (is_post)
  {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
  }
  curl_easy_setopt(curl, CURLOPT_URL, path);


  return slist;
}

static uint32_t telegram_io_read_cb(char *in, uint32_t size, uint32_t nmemb, telegram_io_int_ctx_t *ctx)
{
  uint32_t max_size = MIN(ctx->total_len, (size * nmemb));
  uint32_t ret = 0;

  if (ctx->total_len > 0)
  {
    ret = ctx->cb(ctx->user_data, (uint8_t *)in, max_size, ctx->offset);
    ctx->offset += ret;
    ctx->total_len -= ret;
  }

  return ret;
}

static uint32_t telegram_io_write_cb(char *in, uint32_t size, uint32_t nmemb, telegram_io_mem_t *mem)
{
  uint32_t full_size = size * nmemb;
  char *ptr = realloc(mem->buffer, mem->size + full_size + 1);
  if(ptr == NULL) {
    TELEGRAM_LOGE(TAG,"not enough memory (realloc returned NULL)\r\n");
    return 0;
  }

  mem->buffer = ptr;
  memcpy(&mem->buffer[mem->size], in, full_size);
  mem->size += full_size;
  mem->buffer[mem->size] = 0;

  return full_size;
}

char *telegram_io_send_data_platform(void **io_ctx, const char *path, uint32_t total_len, telegram_io_header_t *headers,
    bool is_post, const char *post_field, void *ctx, telegram_io_send_file_cb_t cb)
{
  CURLcode res;
  static telegram_io_int_ctx_t internal_ctx = {0};
  static telegram_io_mem_t mem = {0};
  CURL *client = NULL;
  struct curl_slist *slist = NULL;

  if (io_ctx)
  {
    client = (CURL *)*io_ctx;
  }

  if (client == NULL)
  {
    client = curl_easy_init();
    if (client == NULL)
    {
        TELEGRAM_LOGE(TAG, "Failed to init http client");
        return NULL;
    }

    if (io_ctx)
    {
        *io_ctx = client;
    }
  }

  slist = telegram_io_prepare_platform(client, path, is_post, headers);
  if (post_field)
  {
    curl_easy_setopt(client, CURLOPT_POSTFIELDS, post_field);
  }

  if ((cb != NULL) && (total_len != 0))
  {
    internal_ctx.cb = cb;
    internal_ctx.user_data = ctx;
    internal_ctx.total_len = total_len;
    curl_easy_setopt(client, CURLOPT_READDATA, &internal_ctx);
    curl_easy_setopt(client, CURLOPT_READFUNCTION, telegram_io_read_cb);
  }

  mem.size = 0;
  mem.buffer = malloc(1);

  curl_easy_setopt(client, CURLOPT_WRITEDATA, &mem);
  curl_easy_setopt(client, CURLOPT_WRITEFUNCTION, telegram_io_write_cb);
  res = curl_easy_perform(client);
  if(res != CURLE_OK)
  {
     mem.size = 0;
     free(mem.buffer);
     mem.buffer = NULL;
     TELEGRAM_LOGE(TAG, "curl_easy_perform failed err %d", res);
  }

  if (io_ctx == NULL)
  {
    telegram_io_free_platform(client);
  }
  curl_slist_free_all(slist);
  return mem.buffer;
}

void telegram_io_read_file(const char *file_path, void *ctx, telegram_io_get_file_cb_t cb)
{
  //TODO
  TELEGRAM_LOGE(TAG, "Not implemented");
}

void telegram_io_free_platform(void *ctx)
{
  if (ctx != NULL)
  {
    curl_easy_cleanup((CURL *)ctx);
  }
}

