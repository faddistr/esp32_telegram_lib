#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <cJSON.h>
#include "telegram_parse.h"

#define TEGLEGRAM_CHAT_ID_MAX_LEN TELEGRAM_INT_MAX_VAL_LENGTH

#define TELEGRAM_INLINE_BTN_FMT "{\"text\": \"%s\", \"callback_data\": \"%s\"}"
#define TELEGRAM_INLINE_KBRD_FMT "\"inline_keyboard\": "

#define TELEGRAM_REPLY_KBRD_HEAD_FMT "\"keyboard\": ["
#define TELEGRAM_REPLY_KBRD_FOOTER_FMT "], \"resize_keyboard\": %s, \"one_time_keyboard\": %s, \"selective\": %s"
#define TELEGRAM_REPLY_KBRD_BTN_FMT "{\"text\": \"%s\", \"request_contact\": %s, \"request_location\": %s}"

#define TELEGRAM_REPLY_KBRD_REMOVE_FMT "\"remove_keyboard\": true, \"selective\": %s"
#define TELEGRAM_FORCE_REPLY_FMT "\"force_reply\": true, \"selective\": %s"

#define TELEGRAM_MSG_FMT "\"chat_id\": \"%.0f\", \"text\": \"%s\""
#define TELEGRAM_MSG_MARKUP_FMT ", \"reply_markup\": {%s}"

#define TELEGRAM_ANSWER_QUERY_FMT_PL "{\"callback_query_id\": \"%s\", \"text\": \"%s\", \"show_alert\": \"%s\", \"url\": \"%s\", \"cache_time\": \"%.0f\"}"

#define TELEGRAM_GET_MESSAGE_POST_DATA_OFFSET_FMT "&offset=%.0f"
#define TELEGRAM_GET_UPDATES_FMT TELEGRAM_SERVER"/bot%s/getUpdates?limit=%d"
#define TELEGRAM_SEND_MESSAGE_FMT  TELEGRAM_SERVER"/bot%s/sendMessage"
#define TELEGRAM_GET_FILE_FMT TELEGRAM_SERVER"/file/bot%s/%s"
#define TELEGRAM_GET_FILE_PATH_FMT  TELEGRAM_SERVER"/bot%s/getFile?file_id=%s"
#define TELEGRAM_SEND_FILE_FMT  TELEGRAM_SERVER"/bot%s/sendDocument"
#define TELEGRAM_SEND_PHOTO_FMT  TELEGRAM_SERVER"/bot%s/sendPhoto"
#define TELEGRAM_ANSWER_QUERY_FMT  TELEGRAM_SERVER"/bot%s/answerCallbackQuery"


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
	char *str = NULL;
	size_t count = 0;
	size_t reqSize = strlen(TELEGRAM_REPLY_KBRD_HEAD_FMT) + strlen(TELEGRAM_REPLY_KBRD_FOOTER_FMT) + 3*strlen("false");
	telegram_kbrd_markup_row_t *row = kbrd->rows;
	telegram_kbrd_btn_t *btn = NULL;

	while (row && row->buttons)
	{
		btn = row->buttons;
		while (btn->text)
		{
			reqSize += strlen(btn->text) + strlen(TELEGRAM_REPLY_KBRD_BTN_FMT) + 1 + 2*strlen("false");
			btn++;
		}

		reqSize += 3; /* [ ] , */

		row++;
	}

	str = calloc(sizeof(char), reqSize);
	if (str == NULL)
	{
		return NULL;
	}

	row = kbrd->rows;
	count = sprintf(str, TELEGRAM_REPLY_KBRD_HEAD_FMT);
	while (row && (row->buttons))
	{
		btn = row->buttons;
		if (btn->text)
		{
			count += sprintf(&str[count], "[");
		}

		while (btn->text)
		{
			count += sprintf(&str[count], TELEGRAM_REPLY_KBRD_BTN_FMT, btn->text, 
				btn->req_contact?"true":"false", btn->req_loc?"true":"false");

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

	sprintf(&str[count], TELEGRAM_REPLY_KBRD_FOOTER_FMT, kbrd->resize?"true":"false", 
		kbrd->one_time?"true":"false", kbrd->selective?"true":"false");

	return str;
}

static char *telegram_make_inline_kbrd(telegram_kbrd_inline_t *kbrd)
{
	char *str = NULL;
	size_t reqSize = 0;
	size_t count = 0;
	size_t row_count = 0;
	telegram_kbrd_inline_row_t *row = kbrd->rows;

	telegram_kbrd_inline_btn_t *btn = NULL;

	while (row && (row->buttons))
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

	while (row && (row->buttons))
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

static char *telegram_make_markup_remove(telegram_kbrd_markup_remove_t *markup_remove)
{
	char *json_res = NULL;

	if (markup_remove == NULL)
	{
		return NULL;
	}

	json_res = calloc(sizeof(char), strlen(TELEGRAM_REPLY_KBRD_REMOVE_FMT) + strlen("false"));
	if (json_res == NULL)
	{
		return NULL;
	}

	sprintf(json_res, TELEGRAM_REPLY_KBRD_REMOVE_FMT, markup_remove->selective?"true":"false");

	return json_res;
}

static char *telegram_make_force_reply(telegram_kbrd_force_reply_t *force_reply)
{
	char *json_res = NULL;

	if (force_reply == NULL)
	{
		return NULL;
	}

	json_res = calloc(sizeof(char), strlen(TELEGRAM_REPLY_KBRD_REMOVE_FMT) + strlen("false"));
	if (json_res == NULL)
	{
		return NULL;
	}

	sprintf(json_res, TELEGRAM_FORCE_REPLY_FMT, force_reply->selective?"true":"false");

	return json_res;
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

		case TELEGRAM_KBRD_MARKUP_REMOVE:
			json_res = telegram_make_markup_remove(&kbrd->kbrd.markup_remove);
			break;

		case TELEGRAM_KBRD_FORCE_REPLY:
			json_res = telegram_make_force_reply(&kbrd->kbrd.force_reply);
			break;

		default:
			return NULL;
	}

	return json_res;
}

char *telegram_make_message(telegram_int_t chat_id, const char *message, telegram_kbrd_t *kbrd)
{
	uint32_t size = strlen(TELEGRAM_MSG_FMT) + TEGLEGRAM_CHAT_ID_MAX_LEN + 2 + 1; /* {} \0 */
	char *additional_json = NULL;
	char *payload = NULL;

	if (!message) /* text field is required  */
	{
		return NULL;
	}

	size += strlen(message);
	if (kbrd)
	{
		additional_json = telegram_make_kbrd(kbrd);

		if (!additional_json)
		{
			return NULL;
		}

		size += strlen(additional_json) + strlen(TELEGRAM_MSG_MARKUP_FMT);
	}

	payload = calloc(sizeof(char), size);
	if (!payload)
	{
		free(additional_json);
		return NULL;
	}

	size = sprintf(payload, "{");	
	size += sprintf(&payload[size], TELEGRAM_MSG_FMT, chat_id, message);
	if (additional_json)
	{
		size += sprintf(&payload[size], TELEGRAM_MSG_MARKUP_FMT, additional_json);
		free(additional_json);
	}

	size += sprintf(&payload[size], "}");	

	return payload;
}

char *telegram_make_answer_query(const char *cid, const char *text, bool show_alert, const char *url, telegram_int_t cache_time)
{
	char *str = NULL;

	if (cid == NULL)
	{
		return NULL;
	}

	str = calloc(sizeof(char), strlen(TELEGRAM_ANSWER_QUERY_FMT_PL) + strlen(cid) + strlen("false")
		+ ((text != NULL)?strlen(text):0) + ((url!= NULL)?strlen(url):0) + TELEGRAM_INT_MAX_VAL_LENGTH);
	if (str == NULL)
	{
		return NULL;
	}

	sprintf(str, TELEGRAM_ANSWER_QUERY_FMT_PL, cid, text?text:"", show_alert?"true":"false", url?url:"", cache_time);

	return str;
}

char *telegram_make_method_path(const telegram_method_t method_id, const char *token, 
	uint32_t limit, telegram_int_t offset, const char *file_id_path)
{
	char *str = NULL;
	if (token == NULL)
	{
		return NULL;
	}

	switch (method_id)
	{
		case TELEGRAM_GET_UPDATES:
			{
				str = calloc(sizeof(char), strlen(TELEGRAM_GET_UPDATES_FMT) + strlen(token) + 1
					+ TELEGRAM_INT_MAX_VAL_LENGTH
					+ (offset?(strlen(TELEGRAM_GET_MESSAGE_POST_DATA_OFFSET_FMT) + TELEGRAM_INT_MAX_VAL_LENGTH):0));
				if (str)
				{
					if (limit == 0)
					{
						limit = TELEGRAM_DEFAULT_MESSAGE_LIMIT;
					}

					sprintf(str, TELEGRAM_GET_UPDATES_FMT, token, limit);
					if (offset)
					{
						sprintf(&str[strlen(str)], TELEGRAM_GET_MESSAGE_POST_DATA_OFFSET_FMT, offset);
					}
				}
			}
			break;

		case TELEGRAM_SEND_MESSAGE:
			{
				str = calloc(sizeof(char), strlen(TELEGRAM_SEND_MESSAGE_FMT) + strlen(token) + 1);
				if (str)
				{
					sprintf(str, TELEGRAM_SEND_MESSAGE_FMT, token);
				}
			}
			break;
			
		case TELEGRAM_GET_FILE_PATH:
			{
				if (!file_id_path)
				{
					return NULL;
				}
				
				str = calloc(sizeof(char), strlen(TELEGRAM_GET_FILE_PATH_FMT) + strlen(file_id_path) 
					+ strlen(token) + 1);
				if (str)
				{
					sprintf(str, TELEGRAM_GET_FILE_PATH_FMT, token, file_id_path);
				}
			}
			break;

		case TELEGRAM_GET_FILE:
			{
				if (!file_id_path)
				{
					return NULL;
				}

				str = calloc(sizeof(char), strlen(TELEGRAM_GET_FILE_FMT) + strlen(token) 
					+ strlen(file_id_path) + 1);
				if (str)
				{
					sprintf(str, TELEGRAM_GET_FILE_FMT, token, file_id_path);
				}
			}
			break;

		case TELEGRAM_SEND_FILE:
			{
				str = calloc(sizeof(char), strlen(TELEGRAM_SEND_FILE_FMT) + strlen(token) + 1);
				if (str)
				{
					sprintf(str, TELEGRAM_SEND_FILE_FMT, token);
				}
			}
			break;

		case TELEGRAM_SEND_PHOTO:
			{
				str = calloc(sizeof(char), strlen(TELEGRAM_SEND_PHOTO_FMT) + strlen(token) + 1);
				if (str)
				{
					sprintf(str, TELEGRAM_SEND_PHOTO_FMT, token);
				}
			}
			break;
			

		case TELEGRAM_ANSWER_QUERY:
			{
				str = calloc(sizeof(char), strlen(TELEGRAM_ANSWER_QUERY_FMT) + strlen(token) + 1);
				if (str)
				{
					sprintf(str, TELEGRAM_ANSWER_QUERY_FMT, token);
				}
			}
			break;
			
		default:
			break;
	}

	return str;
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

