#ifndef TELEGRAM_IO_H
#define TELEGRAM_IO_H

#define TELEGRAM_LONG_POLLING (1)

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

typedef bool(*telegram_io_get_file_cb_t)(void *ctx, uint8_t *buf, int size, int total_len);
void telegram_io_read_file(const char *file_path, void *ctx, telegram_io_get_file_cb_t cb);
#endif /* TELEGRAM_IO_H */