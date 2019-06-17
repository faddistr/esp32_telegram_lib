/**
* Telegram parse module, generates JSON requests and parses JSON responses
*/
#ifndef TELEGRAM_PARSE
#define TELEGRAM_PARSE
#include <stdbool.h>

#define TELEGRAM_SERVER 		"https://api.telegram.org"

/** Telegram int type, 52 bits, uses double for compatibility with cjson lib*/
typedef double telegram_int_t;

#define TELEGRAM_INT_MAX_VAL_LENGTH (64U)

#define TELEGRAM_DEFAULT_MESSAGE_LIMIT (1U)


/** Methods for telegram rest api */
typedef enum
{
	TELEGRAM_GET_UPDATES,
	TELEGRAM_SEND_MESSAGE,
	TELEGRAM_GET_FILE_PATH,
	TELEGRAM_GET_FILE,
	TELEGRAM_SEND_FILE,
	TELEGRAM_SEND_PHOTO,
	TELEGRAM_ANSWER_QUERY,
} telegram_method_t;

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
	const char       *caption;  /** Optional. Caption for the animation, audio, document, photo, video or voice, 0-1024 characters */
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
	TELEGRAM_KBRD_MARKUP_REMOVE,
	TELEGRAM_KBRD_FORCE_REPLY,
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
	telegram_kbrd_btn_t *buttons; /** Array of keyboard buttons, last element should have NULL text field */
} telegram_kbrd_markup_row_t;

typedef struct
{
	telegram_kbrd_markup_row_t *rows; /** Array of arrays of keyboard buttons. Last element should have NULL text field */
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
  telegram_kbrd_inline_btn_t *buttons; /** Array of inline buttons, last element should be NULL */
} telegram_kbrd_inline_row_t;

typedef struct
{
	telegram_kbrd_inline_row_t *rows; /** Array of arrays of inline buttons. Last element should be NULL */
} telegram_kbrd_inline_t;

typedef struct
{
	bool selective;
} telegram_kbrd_markup_remove_t;

typedef struct
{
	bool selective;
} telegram_kbrd_force_reply_t;

typedef union
{
	telegram_kbrd_markup_t markup;
	telegram_kbrd_inline_t inl;
	telegram_kbrd_markup_remove_t markup_remove;
	telegram_kbrd_force_reply_t force_reply;
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
* @return message or NULL
*/
telegram_chat_message_t *telegram_get_message(telegram_update_t *src);


/**
* @brief Parse getFile answer and return file_path
* Memory should be freed
*
* @param buffer where search for a file_path
*
* @return file_path or NULL
*/
char *telegram_parse_file_path(const char *buffer);

/**
* @brief Get chat id or user id from the telegram_chat_message_t structure
*
* @param msg where to search for an id
*
* @return id or -1 - no id found
*/
telegram_int_t telegram_get_chat_id(telegram_chat_message_t *msg);

/**
* @brief Get user id from the telegram_chat_message_t structure
*
* @param msg where to search for an id
*
* @return id or -1 - no id found
*/
telegram_int_t telegram_get_user_id(telegram_chat_message_t *msg);


/**
* @brief Get user id from the telegram_update_t structure
*
* @param src where to search for an id
*
* @return id or -1 - no id found
*/
telegram_int_t telegram_get_user_id_update(telegram_update_t *src);

/**
* @brief Generate message json
*
* @param chat_id id of chat
* @param message text of the message
* @param kbrd optional keyboard object
*
* @return NULL or message
*/
char *telegram_make_message(telegram_int_t chat_id, const char *message, telegram_kbrd_t *kbrd);

/**
* @brief Generate answer query
*
* @param cid id of the query
* @param text optional text of the message
* @param show_alert show alert to user
* @param url optional url
* @param cache_time optional cache time for the answer
*
* @return NULL or answer
*/
char *telegram_make_answer_query(const char *cid, const char *text, bool show_alert, const char *url, 
	telegram_int_t cache_time);

/**
* @brief Generates https REST API method path
*
* @param method_id id of the method, see telegram_method_t
* @param token bot token
* @param limit updates number limit for TELEGRAM_GET_UPDATES
* @param offset update_id from which updates should be received for TELEGRAM_GET_UPDATES
* @param file_id_path file_id for TELEGRAM_GET_FILE_PATH or file_path in case of TELEGRAM_GET_FILE
*
* @return NULL or method
*/
char *telegram_make_method_path(const telegram_method_t method_id, const char *token, 
	uint32_t limit, telegram_int_t offset, const char *file_id_path);
#endif /* TELEGRAM_PARSE */