#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "telegram_parse.h"


#define TELEGRAM_MSG_POLLING_INT 5U
#define TELEGRAM_MAX_TOKEN_LEN 	128U
#define TELEGRAM_SERVER 		"https://api.telegram.org"



typedef enum
{
	TELEGRAM_READ_DATA,
	TELEGRAM_WRITE_DATA,
	TELEGRAM_RESPONSE,
	TELEGRAM_ERR,
	TELEGRAM_END,
} telegram_data_event_t;

typedef struct 
{
	uint8_t  *buf;
	uint32_t pice_size;
	uint32_t total_size;
} telegram_write_data_evt_t;

typedef uint32_t(*telegram_evt_cb_t)(telegram_data_event_t evt, void *teleCtx_ptr, void *ctx, void *evt_data);

void telegram_send_file_e(void *teleCtx_ptr, telegram_int_t chat_id, char *caption, char *filename, uint32_t total_len,
	void *ctx, telegram_evt_cb_t cb);

void telegram_get_file_e(void *teleCtx_ptr, const char *file_id, void *ctx, telegram_evt_cb_t cb);




void telegram_kbrd(void *teleCtx_ptr, telegram_int_t chat_id, const char *message, telegram_kbrd_t *kbrd);
void telegram_send_text_message(void *teleCtx_ptr, telegram_int_t chat_id, const char *message);
void *telegram_init(const char *token, telegram_on_msg_cb_t cb);
void telegram_stop(void *teleCtx);
char *telegram_get_file_path(void *teleCtx_ptr, const char *file_id);

typedef uint32_t(*telegram_send_file_cb_t)(void *ctx, uint8_t *buf, uint32_t max_size);
void telegram_send_file(void *teleCtx_ptr, telegram_int_t chat_id, char *caption, char *filename, uint32_t total_len,
	void *ctx, telegram_send_file_cb_t cb);


typedef bool(*telegram_get_file_cb_t)(void *ctx, uint8_t *buf, int size, int total_len);
void telegram_get_file(void *teleCtx_ptr, const char *file_id, void *ctx, telegram_get_file_cb_t cb);

#endif // TELEGRAM_H