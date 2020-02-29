#ifndef TELEGRAM_HAL_H
#define TELEGRAM_HAL_H
#ifdef ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#define TELEGRAM_SEMAPHORE_t SemaphoreHandle_t
#define TELEGRAM_CREATE_SEMAPHORE(x) vSemaphoreCreateBinary(x)
#define TELEGRAM_DESTROY_SEMAPHORE(x) vSemaphoreDelete(x)
#else
#include <stdlib.h>
#include <pthread.h>

#define TELEGRAM_CREATE_SEMAPHORE(x)
#define TELEGRAM_DESTROY_SEMAPHORE(x)
#endif
#ifdef ESP32
void telegram_wait_mutex_func(TELEGRAM_SEMAPHORE_t *sem, char *func_name);
void telegram_give_mutex_func(TELEGRAM_SEMAPHORE_t *sem);
#endif

#endif // TELEGRAM_HAL_H
