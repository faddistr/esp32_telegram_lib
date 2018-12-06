#include <string.h>
#include <stdbool.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "telegram.h"
#include "telegram_io.h"


#define TELEGRAM_MAX_PATH 128U
#define TEGLEGRAM_CHAT_ID_MAX_LEN 64U

#define TIMER_INTERVAL_MSEC    (1L * 5L * 1000L)

#define TELEGRAM_GET_MESSAGE_POST_DATA_FMT "limit=10"
#define TELEGRAM_GET_MESSAGE_POST_DATA_OFFSET_FMT TELEGRAM_GET_MESSAGE_POST_DATA_FMT"&offset=%d"
#define TELEGRAM_GET_UPDATES_FMT TELEGRAM_SERVER"/bot%s/getUpdates?%s"
#define TELEGRAM_SEND_MESSAGE_FMT  TELEGRAM_SERVER"/bot%s/sendMessage"

#define TELEGRMA_MSG_FMT "{\"chat_id\": \"%f\", \"text\": \"%s\"}"
#define TELEGRMA_MSG_MARKUP_FMT "{\"chat_id\": \"%f\", \"text\": \"%s\", \"reply_markup\": {%s}}"

static const char *TAG="telegram_core";

typedef struct
{
    char *token;	
	int32_t last_update_id;
	bool stop;
	TimerHandle_t timer;
	bool is_timer;
	telegram_on_msg_cb_t on_msg_cb;
} telegram_ctx_t;


static void telegram_process_message_int_cb(void *hnd, telegram_update_t *upd)
{
	telegram_ctx_t *teleCtx = NULL;

	if ((hnd == NULL) || (upd == NULL))
	{
		ESP_LOGE(TAG, "Internal error during processing messages");
		return;
	}

	teleCtx = (telegram_ctx_t *)hnd;
 	teleCtx->last_update_id = upd->id;
 	teleCtx->on_msg_cb(teleCtx, upd);
}

static void telegram_getMessages(telegram_ctx_t *teleCtx)
{
	char *buffer = NULL;
	char *path = NULL;
	char post_data[strlen(TELEGRAM_GET_MESSAGE_POST_DATA_OFFSET_FMT) + 16];

	if (teleCtx->last_update_id)
	{
		sprintf(post_data, TELEGRAM_GET_MESSAGE_POST_DATA_OFFSET_FMT, teleCtx->last_update_id + 1);
	} else
	{
		sprintf(post_data, TELEGRAM_GET_MESSAGE_POST_DATA_FMT);
	}

	path = calloc(sizeof(char), strlen(TELEGRAM_GET_UPDATES_FMT) + strlen(teleCtx->token) + strlen(post_data) + 1);
	if (path == NULL)
	{
		return;
	}

	snprintf(path, TELEGRAM_MAX_PATH, TELEGRAM_GET_UPDATES_FMT, teleCtx->token, post_data);
	buffer = telegram_io_get(path, NULL);
 	if (buffer != NULL)
 	{
 		telegram_parse_messages(teleCtx, buffer, telegram_process_message_int_cb);
 		free(buffer);
 	}

 	free(path);
}

static void telegram_timer_cb(TimerHandle_t pxTimer) 
{
	telegram_ctx_t *teleCtx = (telegram_ctx_t *)pvTimerGetTimerID(pxTimer);
	teleCtx->is_timer = true;
}

static void telegram_task(void * param)
{
	telegram_ctx_t *teleCtx = (telegram_ctx_t *)param;

    ESP_LOGI(TAG, "Start... %s", 	teleCtx->token );
	teleCtx->timer = xTimerCreate("TelegramTimer", TIMER_INTERVAL_MSEC / portTICK_RATE_MS,
        pdTRUE, teleCtx, telegram_timer_cb);
	xTimerStart(teleCtx->timer, 0);

	while(!teleCtx->stop)
	{
		if(teleCtx->is_timer)
		{
			telegram_getMessages(teleCtx);
			teleCtx->is_timer = false;
		}
		vTaskDelay(1);
	}

	free(teleCtx->token);
}

void telegram_stop(void *teleCtx_ptr)
{
	telegram_ctx_t *teleCtx = NULL;

	if (teleCtx_ptr == NULL)
	{
		return;
	}

	teleCtx = (telegram_ctx_t *)teleCtx_ptr;
	teleCtx->stop = true;	
}

void *telegram_init(const char *token, telegram_on_msg_cb_t on_msg_cb)
{
	telegram_ctx_t *teleCtx = NULL;

	if (on_msg_cb == NULL)
	{
		return NULL;
	} 

	teleCtx = calloc(1, sizeof(telegram_ctx_t));
	if (teleCtx != NULL)
	{
		teleCtx->token = strdup(token);
		teleCtx->on_msg_cb = on_msg_cb;
		xTaskCreate(&telegram_task, "telegram_task", 8192, teleCtx, 5, NULL);
	}

	return teleCtx;
}

static void telegram_send_message(void *teleCtx_ptr, telegram_int_t chat_id, const char *message, const char *additional_json)
{
	const telegram_io_header_t sendHeaders[] = 
	{
		{"Content-Type", "application/json"}, 
		{NULL, NULL}
	};
	char *path = NULL;
	char *payload = NULL;
	telegram_ctx_t *teleCtx = NULL;

	if (teleCtx_ptr == NULL)
	{
		ESP_LOGE(TAG, "Send message: Null argument");
		return;
	}

	teleCtx = (telegram_ctx_t *)teleCtx_ptr;
	payload = calloc(sizeof(char), strlen(message) + TEGLEGRAM_CHAT_ID_MAX_LEN 
		+ ((additional_json == NULL) ? strlen(TELEGRMA_MSG_FMT) : (strlen(TELEGRMA_MSG_MARKUP_FMT) + strlen(additional_json))));
	if (payload == NULL)
	{
		return;
	}

	path = calloc(sizeof(char), strlen(TELEGRAM_SEND_MESSAGE_FMT) + strlen(teleCtx->token) + 1);
	if (path == NULL)
	{
		free(payload);
		return;
	}

	sprintf(path, TELEGRAM_SEND_MESSAGE_FMT, teleCtx->token);
	if (additional_json == NULL)
	{
		sprintf(payload, TELEGRMA_MSG_FMT, chat_id, message);
	} else
	{
		sprintf(payload, TELEGRMA_MSG_MARKUP_FMT, chat_id, message, additional_json);
	} 
    ESP_LOGI(TAG, "Send message: %s", payload);

	telegram_io_send(path, payload, (telegram_io_header_t *)sendHeaders); 
	free(path);
	free(payload);
}

void telegram_kbrd(void *teleCtx_ptr, telegram_int_t chat_id, const char *message, telegram_kbrd_t *kbrd)
{
	char *json_res = NULL;

	if ((kbrd == NULL) || (teleCtx_ptr == NULL))
	{
		ESP_LOGE(TAG, "telegram_kbrd ARG is NULL");
		return;
	}

	json_res = telegram_make_kbrd(kbrd);

	if (json_res != NULL)
	{
		telegram_send_message(teleCtx_ptr, chat_id, message, json_res);
		free(json_res);
	}
}

void telegram_send_text_message(void *teleCtx_ptr, telegram_int_t chat_id, const char *message)
{
	telegram_send_message(teleCtx_ptr, chat_id, message, NULL);
}
