#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "telegram_log.h"
#include "telegram_io.h"
#include "telegram_io_hal.h"

static const char *TAG="telegram_io_core";

char *telegram_io_get(const char *path, telegram_io_header_t *headers)
{
    if (path == NULL)
    {
        TELEGRAM_LOGE(TAG, "Wrong arguments(get)");
        return NULL;
    }

    return telegram_io_send_data_platform(NULL, path, 0, headers,  TELEGRAM_HTTP_METHOD_GET,  NULL, NULL, NULL);
}


char *telegram_io_get_ctx(void **io_ctx, const char *path, telegram_io_header_t *headers)
{
    if (path == NULL)
    {
        TELEGRAM_LOGE(TAG, "Wrong arguments(get)");
        return NULL;
    }

    return telegram_io_send_data_platform(io_ctx, path, 0, headers,  TELEGRAM_HTTP_METHOD_GET,  NULL, NULL, NULL);
}

void telegram_io_free_ctx(void **io_ctx)
{
    if (io_ctx == NULL)
    {
        return;
    }

    telegram_io_free_platform(*io_ctx);
    *io_ctx = NULL;
}

void telegram_io_send(const char *path, const char *message, telegram_io_header_t *headers)
{
    char *buffer;
    if ((path == NULL) || (message == NULL))
    {
        TELEGRAM_LOGE(TAG, "Wrong arguments(send)");
        return;
    }

    TELEGRAM_LOGI(TAG, "Send message: %s", message);

    buffer = telegram_io_send_data_platform(NULL, path, 0, headers,  TELEGRAM_HTTP_METHOD_POST,  (char *)message, NULL, NULL);
    free(buffer);
}

char *telegram_io_send_big(const char *path, uint32_t total_len, telegram_io_header_t *headers,
    const char *post_field, void *ctx, telegram_io_send_file_cb_t cb)
{
    if ((path == NULL) || (cb == NULL) || (total_len == 0))
    {
        TELEGRAM_LOGE(TAG, "Wrong arguments(send_big)");
        return NULL;
    }

    return telegram_io_send_data_platform(NULL, path, total_len, headers, TELEGRAM_HTTP_METHOD_POST, (char *)post_field, ctx, cb);
}
