#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "telegram_parse.h"


#define TELEGRAM_MSG_POLLING_INT 5U
#define TELEGRAM_MAX_TOKEN_LEN 	128U
#define TELEGRAM_SERVER 		"https://api.telegram.org"


void telegram_kbrd(void *teleCtx_ptr, telegram_int_t chat_id, const char *message, telegram_kbrd_t *kbrd);
void telegram_send_text_message(void *teleCtx_ptr, telegram_int_t chat_id, const char *message);
void *telegram_init(const char *token, telegram_on_msg_cb_t cb);
void telegram_stop(void *teleCtx);
char *telegram_get_file_path(void *teleCtx_ptr, const char *file_id);
typedef uint32_t(*send_file_cb_t)(void *ctx, uint8_t *buf, uint32_t max_size);
//void telegram_send_file(void *teleCtx_ptr, telegram_int_t chat_id, char *caption, void *ctx, send_file_cb_t cb);
#endif // TELEGRAM_H