#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_log.h>
#include "telegram_getter.h"

static const char *TAG="telegram_esp_get";

typedef struct
{
	bool stop;
#if TELEGRAM_LONG_POLLING != 1
	TimerHandle_t timer;
	bool is_timer;
#endif
	TaskHandle_t task;	
	telegram_getMessages_t onGetMessages;
	void *ctx;
} telegram_getter_t;

#if TELEGRAM_LONG_POLLING != 1
static void telegram_timer_cb(TimerHandle_t pxTimer) 
{
	telegram_getter_t *teleCtx = (telegram_getter_t *)pvTimerGetTimerID(pxTimer);
	teleCtx->is_timer = true;
}
#endif

static void telegram_task(void * param)
{
	telegram_getter_t *teleCtx = (telegram_getter_t *)param;

    ESP_LOGI(TAG, "Start... thread");
	while(!teleCtx->stop)
	{
#if TELEGRAM_LONG_POLLING == 1
		teleCtx->onGetMessages(teleCtx->ctx);
#else
		if(teleCtx->is_timer)
		{
			teleCtx->onGetMessages(teleCtx->ctx);
			teleCtx->is_timer = false;
		}
#endif
		vTaskDelay(1);
	}

}

void *telegram_getter_init(telegram_getMessages_t onGetMessages, void *ctx)
{
	telegram_getter_t *teleCtx = NULL;

	if (onGetMessages == NULL)
	{
		return NULL;
	} 

	teleCtx = calloc(1, sizeof(telegram_getter_t));
	if (teleCtx != NULL)
	{
		teleCtx->onGetMessages = onGetMessages;
		teleCtx->ctx = 	ctx;
#if TELEGRAM_LONG_POLLING != 1
		teleCtx->timer = xTimerCreate("TelegramTimer", TIMER_INTERVAL_MSEC / portTICK_RATE_MS,
        	pdTRUE, teleCtx, telegram_timer_cb);
		xTimerStart(teleCtx->timer, 0);
#endif
		xTaskCreate(&telegram_task, "telegram_task", 5120, teleCtx, 5, &teleCtx->task);
	}

	return teleCtx;
}

void telegram_getter_stop(void *teleCtx_ptr)
{
	telegram_getter_t *teleCtx = (telegram_getter_t *)teleCtx_ptr;

	if (teleCtx_ptr == NULL)
	{
		return;
	}


#if TELEGRAM_LONG_POLLING != 1
	xTimerStop(teleCtx->timer, 0);
#endif
	vTaskDelete(teleCtx->task);
#if TELEGRAM_LONG_POLLING != 1
	xTimerDelete(teleCtx->timer, 0);
#endif
	free(teleCtx);
}
