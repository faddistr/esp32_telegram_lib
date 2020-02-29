#ifndef TELEGRAM_LOG_H
#define TELEGRAM_LOG_H
#ifdef ESP32
#include <esp_log.h>
#define TELEGRAM_LOGI(TAG, ...) ESP_LOGI(TAG, __VA_ARGS__)
#define TELEGRAM_LOGE(TAG, ...) ESP_LOGE(TAG, __VA_ARGS__)
#define TELEGRAM_LOGW(TAG, ...) ESP_LOGW(TAG, __VA_ARGS__)
#else
#include <syslog.h>

#define TELEGRAM_LOGI(TAG, ...) do {\
                     syslog(LOG_NOTICE, "%s: ", TAG);\
                     syslog(LOG_NOTICE, __VA_ARGS__);\
               } while(0)
#define TELEGRAM_LOGE(TAG, ...) do {\
                     syslog(LOG_ERR, "%s: ", TAG);\
                     syslog(LOG_ERR, __VA_ARGS__); \
               } while(0)

#define TELEGRAM_LOGW(TAG, ...) do {\
                     syslog(LOG_WARNING, "%s: ", TAG);\
                     syslog(LOG_WARNING, __VA_ARGS__); \
               } while(0)
#endif


#endif // TELEGRAM_LOG_H
