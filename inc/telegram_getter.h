#ifndef TELEGRAM_GETTER_H
#define TELEGRAM_GETTER_H
#include "telegram_io.h"

#if TELEGRAM_LONG_POLLING != 1
#define TIMER_INTERVAL_MSEC    (1L * 5L * 1000L)
#endif
typedef void(* telegram_getMessages_t)(void *teleCtx);

void *telegram_getter_init(telegram_getMessages_t onGetMessages, void *ctx);

void telegram_getter_stop(void *ctx);

#endif /* TELEGRAM_GETTER_H */