/**

*/
#ifndef TELEGRAM_PARSE
#define TELEGRAM_PARSE
#include <stdbool.h>

/** Telegram int type, 52 bits, uses double for compatibility with cjson lib*/
typedef double telegram_int_t;
struct telegram_chat_message;

/** This object represents a Telegram user or bot */
typedef struct
{
	telegram_int_t id;             /** Unique identifier for this user or bot */
	bool is_bot;                   /** True, if this user is a bot */    
	const char *first_name;        /** User‘s or bot’s first name */
	const char *last_name;         /** Opt. User‘s or bot’s last name */
	const char *username;          /** Opt. User‘s or bot’s username */
	const char *language_code;     /** Opt. IETF language tag of the user's language */
} telegram_user_t;

/** Type of the chat possible values */
typedef enum
{
	TELEGRAM_CHAT_TYPE_UNIMPL,
	TELEGRAM_CHAT_TYPE_PRIVATE,
	TELEGRAM_CHAT_TYPE_CHAN,
	TELEGRAM_CHAT_TYPE_GROUP,
	TELEGRAM_CHAT_TYPE_SUPERGROUP,
	TELEGRAM_CHAT_TYPE_COUNT
} telegram_chat_type_t;

/** This object represents a chat. */
typedef struct
{
	telegram_int_t id; /** 	Unique identifier for this chat. */
	const char *title; /**  Opt. Title of the chat  */ 
	telegram_chat_type_t type; /** Type of the chat */
	struct telegram_chat_message  *pinned_message; /** Opt. Pinned message, for supergroups and channel chats */
} telegram_chat_t;

/** This object represents one size of a photo or a file / sticker thumbnail. */
typedef struct
{
	const char *id; /** Unique file identifier */
	telegram_int_t width; /** Photo width */
	telegram_int_t height; /** Photo height */
	telegram_int_t file_size; /** Opt. file size */
} telegram_photosize_t;

/** This object represents a general file */
typedef struct
{
	const char *id; /** Unique file identifier */
	telegram_photosize_t *thumb; /** Opt.  Document thumbnail as defined by sender */
	const char *name; /** Opt. Original filename as defined by sender */
	const char *mime_type; /** Opt. MIME type of the file as defined by sender */
	telegram_int_t file_size; /** Opt. File size */
} telegram_document_t; 

/** This object represents a message. */
typedef struct telegram_chat_message
{
	telegram_int_t   id;        /** Unique message identifier inside this chat */
	telegram_user_t  *from;     /** NULL in case of channel posts */
	telegram_int_t   timestamp; /** Unix timestamp of the message */
	telegram_chat_t  *chat;     /** Conversation the message belongs to */
	telegram_user_t  *forward_from; /** Opt. For forwarded messages, sender of the original message */
	telegram_chat_t  *forward_from_chat; /** Opt. For forwarded messages, chat of the original message */
	telegram_int_t   forward_from_message_id; /** Opt. For messages forwarded from channels, identifier of the original message in the channel */
	const char       *forward_signature; /** Opt. For messages forwarded from channels, signature of the post author if present*/
	telegram_int_t   forward_date; /** Opt. For forwarded messages, date the original message was sent in Unix time*/
	struct telegram_chat_message *reply_to_message; /** Opt. For replies, the original message. Note that the Message object in this field will not contain further reply_to_message fields even if it itself is a reply.*/
	telegram_int_t   edit_date; /** Opt.  Date the message was last edited in Unix time */
	const char       *media_group_id; /** Opt. The unique identifier of a media message group this message belongs to */
	const char       *author_signature; /** Opt. Signature of the post author for messages in channels */
	const char       *text;     /** Opt. Text of the message */
	telegram_document_t *file; /** Opt. General file */
} telegram_chat_message_t;

/** This object represents an incoming callback query from a callback button in an inline keyboard. */
typedef struct
{
	const char                   *id;         /** Unique identifier for this query */
	telegram_user_t              *from;       /** Sender */
	const char      			 *data;       /** Opt. Data associated with the callback button. */
	telegram_chat_message_t      *message;    /** Opt. Message that was send to user */
} telegram_chat_callback_t;

/** This object represents an incoming update. */
typedef struct
{
	telegram_int_t               id;                   /** The update‘s unique identifier. */
	telegram_chat_message_t      *message;             /** Opt. New incoming message */
	telegram_chat_message_t      *edited_message;      /** Opt. New version of a message that is known to the bot and was edited */
	telegram_chat_message_t      *channel_post;        /** Opt. Message that was send to user */
	telegram_chat_message_t		 *edited_channel_post; /** Opt. New version of a channel post that is known to the bot and was edited */
	telegram_chat_callback_t     *callback_query;      /** New incoming callback query */
} telegram_update_t;

/** Callback on single message parser */
typedef void(*telegram_on_msg_cb_t)(void *teleCtx, telegram_update_t *info);

/** Telegram keyboard types */
typedef enum
{
	TELEGRAM_KBRD_MARKUP,
	TELEGRAM_KBRD_INLINE,
	TELEGRAM_KBRD_COUNT
}telegram_kbrd_type_t;

typedef struct 
{
	char *text;
	bool req_contact;
	bool req_loc;
} telegram_kbrd_btn_t;

typedef struct
{
	telegram_kbrd_btn_t *buttons; /** Button array. Last element should be NULL */
	bool resize;
	bool one_time;
	bool selective;
} telegram_kbrd_markup_t;

typedef struct 
{
	char *text;
	char callback_data[64 + 1];
} telegram_kbrd_inline_btn_t;

typedef struct
{
	telegram_kbrd_inline_btn_t *buttons; /** Button array. Last element should be NULL */
} telegram_kbrd_inline_t;

typedef union
{
	telegram_kbrd_markup_t markup;
	telegram_kbrd_inline_t inl;
} telegram_kbrd_descr_t;

typedef struct 
{
	telegram_kbrd_type_t type;
	telegram_kbrd_descr_t kbrd;
} telegram_kbrd_t;

/**
* @brief Parse income array of messages
* All allocated memory will be freed internaly
* @param teleCtx pointer on internal telegram structure, used as argument in callback
* @param buffer string with JSON array
* @param cb callback to call on each message
*
* @return none
*/
void telegram_parse_messages(void *teleCtx, const char *buffer, telegram_on_msg_cb_t cb);

/**
* @brief Parse telegram_kbrd_t into JSON object
* Memory should be free with free function
*
* @param kbrd pointer on C structure that is described keyboard
*
* @return string representation of the JSON object
*/
char *telegram_make_kbrd(telegram_kbrd_t *kbrd);

/**
* @brief Get a message from telegram_update_t structure
*
* @param src where search for a message
*
* @retrun message or NULL
*/
telegram_chat_message_t *telegram_get_message(telegram_update_t *src);


/**
* @brief Get chat id or user id from the telegram_chat_message_t structure
*
* @param msg where search for an id
*
* @retrun id or -1 - no id found
*/
telegram_int_t telegram_get_chat_id(telegram_chat_message_t *msg);
#endif /* TELEGRAM_PARSE */