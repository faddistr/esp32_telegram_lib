#include "telegram.h"

telegram_chat_message_t *telegram_get_message(telegram_update_t *src)
{
	if (src == NULL)
	{
		return NULL;
	}

	if (src->message != NULL)
	{
		return src->message;
	}

	if (src->channel_post != NULL)
	{
		return src->channel_post;
	}

	return NULL;
}

telegram_int_t telegram_get_user_id(telegram_chat_message_t *msg)
{
	if (msg == NULL)
	{
		return -1;
	}
	
	if (msg->from != NULL)
	{
		return msg->from->id;
	}

	return -1;
}

telegram_int_t telegram_get_user_id_update(telegram_update_t *src)
{
	telegram_chat_message_t *msg = telegram_get_message(src);
	telegram_chat_callback_t *cb = src->callback_query;
	if ((msg != NULL) && (msg->from != NULL))
	{
		return msg->from->id;
	}
	
	if (cb != NULL)
	{
		return cb->from->id;
	}

	return -1;
}

telegram_int_t telegram_get_chat_id(telegram_chat_message_t *msg)
{
	if (msg == NULL)
	{
		return -1;
	}

	if (msg->chat != NULL)
	{
		return msg->chat->id;
	}

	return telegram_get_user_id(msg);
}
