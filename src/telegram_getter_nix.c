#include <stdbool.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include "telegram_getter.h"
#include "telegram_log.h"
static const char *TAG="telegram_nix_get";

typedef struct
{
  bool should_stop;
  pthread_t thread;
  void *user_ctx;
  telegram_getMessages_t onGetMsg_cb;
} telegram_getter_nix_t;

static void *telegram_getter_thread(void *args)
{
  telegram_getter_nix_t *ictx = (telegram_getter_nix_t *)args;

  while(!ictx->should_stop)
  {
    ictx->onGetMsg_cb(ictx->user_ctx);
    usleep(1000);
  }
  return NULL;
}

void *telegram_getter_init(telegram_getMessages_t onGetMessages, void *ctx)
{
  int status;
  telegram_getter_nix_t *int_ctx;

  if (onGetMessages == NULL)
  {
    TELEGRAM_LOGE(TAG, "Incorrect params onGetMessages == NULL");
    return NULL;
  }

  int_ctx = calloc(sizeof(telegram_getter_nix_t), 1);
  if (int_ctx == NULL)
  {
      TELEGRAM_LOGE(TAG, "No mem");
      return NULL;
  }

  int_ctx->user_ctx = ctx;
  int_ctx->onGetMsg_cb = onGetMessages;
  status = pthread_create(&int_ctx->thread, NULL, telegram_getter_thread, int_ctx);
  if (status != 0)
  {
      TELEGRAM_LOGE(TAG, "Failed to init thread. Err: %d", status);
      free(int_ctx);
      return NULL;
  }
  return int_ctx;
}

void telegram_getter_stop(void *ctx)
{
  telegram_getter_nix_t *ictx = (telegram_getter_nix_t *)ctx;

  if (ctx == NULL)
  {
    return;
  }

  ictx->should_stop = true;
  (void)pthread_join(ictx->thread, NULL);
  free(ctx);
}
