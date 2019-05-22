#ifndef TELEGRAM_IO_H
#define TELEGRAM_IO_H

typedef struct
{
	const char *key;
	const char *value;
} telegram_io_header_t;

typedef uint32_t(*telegram_io_send_file_cb_t)(void *ctx, uint8_t *buf, uint32_t max_size);

/**
 headers should be end with null key
*/

char *telegram_io_get(const char *path, telegram_io_header_t *headers);

void telegram_io_send(const char *path, const char *message, telegram_io_header_t *headers);

char *telegram_io_send_big(const char *path, uint32_t total_len, telegram_io_header_t *headers, 
    const char *post_field, void *ctx, telegram_io_send_file_cb_t cb);

#endif /* TELEGRAM_IO_H */