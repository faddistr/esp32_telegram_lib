#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "telegram_hal.h"
#include "telegram_log.h"

void telegram_wait_mutex_func(TELEGRAM_SEMAPHORE_t *sem, char *func_name)
{
    while (!xSemaphoreTake(sem, portMAX_DELAY))
    {
        TELEGRAM_LOGW(TAG, "Mutex wait error! %s", func_name);
    }
}

void telegram_give_mutex_func(TELEGRAM_SEMAPHORE_t *sem)
{
  xSemaphoreGive(sem);
}
