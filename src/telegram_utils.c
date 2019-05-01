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

telegram_int_t telegram_get_chat_id(telegram_chat_message_t *msg)
{
	if (msg == NULL)
	{
		return -1;
	}

	if (msg->from != NULL)
	{
		return msg->from->id;
	}

	if (msg->chat != NULL)
	{
		return msg->chat->id;
	}


	return -1;
}