#ifndef TELEGRAM_IO_H
#define TELEGRAM_IO_H

typedef struct
{
	const char *key;
	const char *value;
} telegram_io_header_t;

char *telegram_io_get(const char *path, const char *post_data);
/**
 header should be end with null key
*/
void telegram_io_send(const char *path, const char *message, telegram_io_header_t *header);
#endif