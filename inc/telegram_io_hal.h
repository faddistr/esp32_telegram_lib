#ifndef TELEGRAM_IO_SEND_H
#define TELEGRAM_IO_SEND_H
#include <stdint.h>
#include <stdbool.h>
#include "telegram_io.h"

#define TELEGRAM_HTTP_METHOD_GET false
#define TELEGRAM_HTTP_METHOD_POST true

void telegram_io_free_platform(void *ctx);

char *telegram_io_send_data_platform(void **io_ctx, const char *path, uint32_t total_len, telegram_io_header_t *headers,
    bool is_post, const char *post_field, void *ctx, telegram_io_send_file_cb_t cb);

#endif // TELEGRAM_IO_SEND_H
