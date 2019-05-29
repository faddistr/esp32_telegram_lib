#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <cJSON.h>
#include "telegram_parse.h"

#define TELEGRAM_INLINE_BTN_FMT "{\"text\": \"%s\", \"callback_data\": \"%s\"}"
#define TELEGRAM_INLINE_KBRD_FMT "\"inline_keyboard\": "

static void telegram_free_user(telegram_user_t *user);
static void telegram_free_chat(telegram_chat_t *chat);
static void telegram_free_file(telegram_document_t *file);
static void telegram_free_photosize(telegram_photosize_t *file);
static void telegram_free_message(telegram_chat_message_t *msg);
static void telegram_free_callback_query(telegram_chat_callback_t *query);
static telegram_chat_type_t telegram_get_chat_type(const char *strType);
static telegram_user_t *telegram_parse_user(cJSON *subitem);
static telegram_chat_t *telegram_parse_chat(cJSON *subitem);
static telegram_chat_message_t *telegram_parse_message(cJSON *subitem);
static telegram_chat_callback_t *telegram_parse_callback_query(cJSON *subitem);

static void telegram_free_user(telegram_user_t *user)
{
	free(user);
}

static void telegram_free_chat(telegram_chat_t *chat)
{
	if (chat == NULL)
	{
		return;
	}

	telegram_free_message(chat->pinned_message);
	free(chat);
}

static void telegram_free_photosize(telegram_photosize_t *file)
{
	free(file);
}

static void telegram_free_file(telegram_document_t *file)
{
	if (file == NULL)
	{
		return;
	}

	telegram_free_photosize(file->thumb);
	free(file);
}

static void telegram_free_message(telegram_chat_message_t *msg)
{
	if (msg == NULL)
	{
		return;
	}

	telegram_free_user(msg->from);
	telegram_free_user(msg->forward_from);
	telegram_free_chat(msg->chat);
	telegram_free_chat(msg->forward_from_chat);
	telegram_free_message(msg->reply_to_message);
	telegram_free_file(msg->file);
	free(msg);
}

static void telegram_free_callback_query(telegram_chat_callback_t *query)
{
	if (query == NULL)
	{
		return;
	}

	telegram_free_user(query->from);
	telegram_free_message(query->message);
	free(query);
}


static telegram_chat_type_t telegram_get_chat_type(const char *strType)
{
	if (!strcmp(strType, "private"))
	{
		return TELEGRAM_CHAT_TYPE_PRIVATE;
	}

	if (!strcmp(strType, "channel"))
	{
		return TELEGRAM_CHAT_TYPE_CHAN;
	}

	if (!strcmp(strType, "group"))
	{
		return TELEGRAM_CHAT_TYPE_GROUP;
	}

	if (!strcmp(strType, "supergroup"))
	{
		return TELEGRAM_CHAT_TYPE_SUPERGROUP;
	}

	return TELEGRAM_CHAT_TYPE_UNIMPL;
}

static telegram_chat_t *telegram_parse_chat(cJSON *subitem)
{
	cJSON *val = NULL;
	telegram_chat_t *chat = calloc(1, sizeof(telegram_chat_t));

	if (chat == NULL)
	{
		return NULL;
	}

	val = cJSON_GetObjectItem(subitem, "id");
	if (val != NULL)
	{
		chat->id = val->valuedouble; 
	}

	val = cJSON_GetObjectItem(subitem, "title");
	if (val != NULL)
	{
		chat->title = val->valuestring; 
	}

	val = cJSON_GetObjectItem(subitem, "type");
	if (val != NULL)
	{
		chat->type = telegram_get_chat_type(val->valuestring);
	}

	val = cJSON_GetObjectItem(subitem, "pinned_message");
	if (val != NULL)
	{
		chat->pinned_message = telegram_parse_message(val);
	}

	return chat;
}

static telegram_photosize_t *telegram_parse_photosize(cJSON *subitem)
{
	cJSON *val = NULL;
	telegram_photosize_t *photosize = calloc(1, sizeof(telegram_photosize_t));

	if (photosize == NULL)
	{
		return NULL;
	}

	val = cJSON_GetObjectItem(subitem, "file_id");
	if (val != NULL)
	{
		photosize->id = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "width");
	if (val != NULL)
	{
		photosize->width = val->valuedouble;
	}

	val = cJSON_GetObjectItem(subitem, "height");
	if (val != NULL)
	{
		photosize->height = val->valuedouble;
	}

	val = cJSON_GetObjectItem(subitem, "file_size");
	if (val != NULL)
	{
		photosize->file_size = val->valuedouble;
	}

	return photosize;
}

static telegram_document_t *telegram_parse_file(cJSON *subitem)
{
	cJSON *val = NULL;
	telegram_document_t *file = calloc(1, sizeof(telegram_document_t));

	if (file == NULL)
	{
		return NULL;
	}

	val = cJSON_GetObjectItem(subitem, "file_id");
	if (val != NULL)
	{
		file->id = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "thumb");
	if (val != NULL)
	{
		file->thumb = telegram_parse_photosize(val);
	}

	val = cJSON_GetObjectItem(subitem, "file_name");
	if (val != NULL)
	{
		file->name = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "mime_type");
	if (val != NULL)
	{
		file->mime_type = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "file_size");
	if (val != NULL)
	{
		file->file_size = val->valuedouble;
	}

	return file;
}

static telegram_chat_message_t *telegram_parse_message(cJSON *subitem)
{
	cJSON *val = NULL;
	telegram_chat_message_t *msg = calloc(1, sizeof(telegram_chat_message_t));

	if (msg == NULL)
	{
		return NULL;
	}

	val = cJSON_GetObjectItem(subitem, "message_id");
	if (val != NULL)
	{
		msg->id = val->valuedouble;
	}

	val = cJSON_GetObjectItem(subitem, "from");
	if (val != NULL)
	{
		msg->from = telegram_parse_user(val);
	}

	val = cJSON_GetObjectItem(subitem, "date");
	if (val != NULL)
	{
		msg->timestamp = val->valuedouble;
	}

	val = cJSON_GetObjectItem(subitem, "chat");
	if (val != NULL)
	{
		msg->chat = telegram_parse_chat(val);
	}

	val = cJSON_GetObjectItem(subitem, "forward_from");
	if (val != NULL)
	{
		msg->forward_from = telegram_parse_user(val);
	}

	val = cJSON_GetObjectItem(subitem, "forward_from_chat");
	if (val != NULL)
	{
		msg->forward_from_chat = telegram_parse_chat(val);
	}

	val = cJSON_GetObjectItem(subitem, "forward_from_message_id");
	if (val != NULL)
	{
		msg->forward_from_message_id = val->valuedouble;
	}

	val = cJSON_GetObjectItem(subitem, "forward_signature");
	if (val != NULL)
	{
		msg->forward_signature = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "forward_date");
	if (val != NULL)
	{
		msg->forward_date = val->valuedouble;
	}

	val = cJSON_GetObjectItem(subitem, "reply_to_message");
	if (val != NULL)
	{
		msg->reply_to_message = telegram_parse_message(val);
	}

	val = cJSON_GetObjectItem(subitem, "edit_date");
	if (val != NULL)
	{
		msg->edit_date = val->valuedouble;
	}

	val = cJSON_GetObjectItem(subitem, "media_group_id");
	if (val != NULL)
	{
		msg->media_group_id = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "author_signature");
	if (val != NULL)
	{
		msg->author_signature = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "text");
	if (val != NULL)
	{
		msg->text = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "document");
	if (val != NULL)
	{
		msg->file = telegram_parse_file(val);
	}

	val = cJSON_GetObjectItem(subitem, "caption");
	if (val != NULL)
	{
		msg->caption = val->valuestring;
	}

	return msg;
}

static telegram_chat_callback_t *telegram_parse_callback_query(cJSON *subitem)
{
	cJSON *val = NULL;
	telegram_chat_callback_t *cb = calloc(1, sizeof(telegram_chat_callback_t));
	
	if (cb == NULL)
	{
		return NULL;
	}

	val = cJSON_GetObjectItem(subitem, "id");
	if (val != NULL)
	{
		cb->id = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "from");
	if (val != NULL)
	{
		cb->from = telegram_parse_user(val);
	}

	val = cJSON_GetObjectItem(subitem, "data");
	if (val != NULL)
	{
		cb->data = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "message");
	if (val != NULL)
	{
		cb->message = telegram_parse_message(val);
	}

	return cb;
}

static telegram_user_t *telegram_parse_user(cJSON *subitem)
{
	cJSON *val = NULL;
	telegram_user_t *user = calloc(1, sizeof(telegram_user_t));

	if (user == NULL)
	{
		return NULL;
	}

	val = cJSON_GetObjectItem(subitem, "id");
	if (val != NULL)
	{
		user->id = val->valuedouble;
	}

	val = cJSON_GetObjectItem(subitem, "is_bot");
	if (val != NULL)
	{
		user->is_bot = (val->valueint == 1);
	}

	val = cJSON_GetObjectItem(subitem, "first_name");
	if (val != NULL)
	{
		user->first_name = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "last_name");
	if (val != NULL)
	{
		user->last_name = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "username");
	if (val != NULL)
	{
		user->username = val->valuestring;
	}

	val = cJSON_GetObjectItem(subitem, "language_code");
	if (val != NULL)
	{
		user->language_code = val->valuestring;
	}

	return user;
}

static telegram_update_t *telegram_parse_update(cJSON *subitem)
{
	telegram_update_t *upd = NULL;
	cJSON *val = NULL;

	if (subitem == NULL)
	{
		return NULL;
	}

	upd = calloc(1, sizeof(telegram_update_t));
	if (upd == NULL)
	{
		return NULL;
	}

	val = cJSON_GetObjectItem(subitem, "update_id");
	if (val != NULL)
	{
		upd->id = val->valuedouble;
	} else
	{
		val = cJSON_GetObjectItem(subitem, "message_id");
		if (val != NULL)
		{
			upd->message = telegram_parse_message(subitem);
		}

		return upd;
	}

	val = cJSON_GetObjectItem(subitem, "message");
	if (val != NULL)
	{
		upd->message = telegram_parse_message(val);
	}

	val = cJSON_GetObjectItem(subitem, "edited_message");
	if (val != NULL)
	{
		upd->edited_message = telegram_parse_message(val);
	}

	val = cJSON_GetObjectItem(subitem, "channel_post");
	if (val != NULL)
	{
		upd->channel_post = telegram_parse_message(val);
	}

	val = cJSON_GetObjectItem(subitem, "edited_channel_post");
	if (val != NULL)
	{
		upd->channel_post = telegram_parse_message(val);
	}

	val = cJSON_GetObjectItem(subitem, "callback_query");
	if (val != NULL)
	{
		upd->callback_query = telegram_parse_callback_query(val);
	}

	return upd;
}

static void telegram_free_update_info(telegram_update_t **upd)
{
	telegram_update_t *info = *upd;

	if (*upd == NULL)
	{
		return;
	}

	telegram_free_message(info->message);
	telegram_free_message(info->channel_post);
	telegram_free_message(info->edited_message);
	telegram_free_message(info->edited_channel_post);
	telegram_free_callback_query(info->callback_query);
	free(*upd);
	*upd = NULL;
}

static void telegram_process_messages(void *teleCtx, cJSON *messages, telegram_on_msg_cb_t cb)
{
	uint32_t num_messages = 1;
	cJSON *subitem = messages;
	telegram_update_t *upd = NULL;
	uint32_t i;

	if (cJSON_IsArray(messages))
	{
		num_messages = cJSON_GetArraySize(messages);
	}

	for (i = 0 ; i < num_messages; i++)
	{
		if (cJSON_IsArray(messages))
		{
			subitem = cJSON_GetArrayItem(messages, i);
		}

		if (subitem == NULL)
		{
			break;
		}

		upd = telegram_parse_update(subitem);
		if (upd != NULL)
		{
			cb(teleCtx, upd);
			telegram_free_update_info(&upd);
		}
	}
}

static char *telegram_make_markup_kbrd(telegram_kbrd_markup_t *kbrd)
{
	return NULL;
}

static char *telegram_make_inline_kbrd(telegram_kbrd_inline_t *kbrd)
{
	char *str = NULL;
	size_t reqSize = 0;
	size_t count = 0;
	size_t row_count = 0;
	telegram_kbrd_inline_row_t *row = kbrd->rows;

	telegram_kbrd_inline_btn_t *btn = NULL;

	while (row->buttons)
	{
		btn = row->buttons;
		while (btn->text)
		{
			reqSize += strlen(btn->text) + strlen(btn->callback_data);
			btn++;
			count++;
		}

		row++;
		row_count++;
	}

	if (reqSize == 0)
	{
		return NULL;
	}

	str = (char *)calloc(sizeof(char), reqSize + count*strlen(TELEGRAM_INLINE_BTN_FMT) + count + 1
		+ strlen(TELEGRAM_INLINE_KBRD_FMT) + 2 + 2*row_count + 1); //count +1 number of comas, 2 - brackets

	if (str == NULL)
	{
		return NULL;
	}

	count = sprintf(str, TELEGRAM_INLINE_KBRD_FMT"[");
	row = kbrd->rows;

	while(row->buttons)
	{
		btn = row->buttons;
		count += sprintf(&str[count], "[");
		while (btn->text)
		{
			count += sprintf(&str[count], TELEGRAM_INLINE_BTN_FMT, btn->text, btn->callback_data);
			btn++;
			if (btn->text)
			{
				count += sprintf(&str[count], ",");
			}
		}

		count += sprintf(&str[count], "]");
		row++;
		if (row->buttons)
		{
			count += sprintf(&str[count], ",");
		}
	}

	sprintf(&str[count], "]");
	return str;
}

char *telegram_make_kbrd(telegram_kbrd_t *kbrd)
{
	char *json_res = NULL;

	if (kbrd == NULL)
	{
		return NULL;
	}

	switch (kbrd->type)
	{
		case TELEGRAM_KBRD_MARKUP:
			json_res = telegram_make_markup_kbrd(&kbrd->kbrd.markup);
			break;

		case TELEGRAM_KBRD_INLINE:
			json_res = telegram_make_inline_kbrd(&kbrd->kbrd.inl);
			break;

		default:
			return NULL;
	}

	return json_res;
}

void telegram_parse_messages(void *teleCtx, const char *buffer, telegram_on_msg_cb_t cb)
{
	cJSON *json = NULL;
	cJSON *ok_item = NULL;
	if ((buffer == NULL) || (cb == NULL))
	{
		return;
	}

	json = cJSON_Parse(buffer);
	if (json == NULL)
	{
		return;
	}

	ok_item = cJSON_GetObjectItem(json, "ok");
	if  ((ok_item != NULL) && (cJSON_IsBool(ok_item) && (ok_item->valueint)))
	{
		cJSON *messages = cJSON_GetObjectItem(json, "result");
		if (messages != NULL)
		{
			telegram_process_messages(teleCtx, messages, cb);
		}
	}

	cJSON_Delete(json);
}

char *telegram_parse_file_path(const char *buffer)
{
	char *ret = NULL;
	cJSON *json = NULL;
	cJSON *ok_item = NULL;

	if (buffer == NULL)
	{
		return NULL;
	}

	json = cJSON_Parse(buffer);
	if (json == NULL)
	{
		return NULL;
	}

	ok_item = cJSON_GetObjectItem(json, "ok");
	if  ((ok_item != NULL) && (cJSON_IsBool(ok_item) && (ok_item->valueint)))
	{
		cJSON *file_info = cJSON_GetObjectItem(json, "result");
		if (file_info != NULL)
		{
			cJSON *file_path = cJSON_GetObjectItem(file_info, "file_path");

			if (file_path != NULL)
			{
				ret = strdup(file_path->valuestring);
			}
		}
	}

	cJSON_Delete(json);
	return ret;
}