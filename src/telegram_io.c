#include <string.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include "telegram_io.h"
#define TELGRAM_DBG 1

#define TELEGRAM_MAX_BUFFER 4095U


#define min(x, y) (((x) < (y))?(x):(y))

static const char *TAG="telegram_io";
#if TELGRAM_DBG == 1
static esp_err_t telegram_http_event_handler(esp_http_client_event_t *evt);
#endif

static const esp_http_client_config_t telegram_io_http_cfg = 
{
#if TELGRAM_DBG == 1
        .event_handler = telegram_http_event_handler,
#endif
        .timeout_ms = 5000,
        .url = "http://example.org",
};
#if TELGRAM_DBG == 1
static esp_err_t telegram_http_event_handler(esp_http_client_event_t *evt)
{
      switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}
#endif

static char *telegram_io_read_all_content(esp_http_client_handle_t client)
{
    int data_read;
    char *buffer = NULL;
    uint32_t content_length;
    uint32_t total_data_read = 0;

    content_length =  esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
        esp_http_client_get_status_code(client), content_length); 
    
    if ((content_length > TELEGRAM_MAX_BUFFER) || (content_length <= 0))
    {
        return NULL;
    }     

    buffer = calloc(1, content_length + 1);
    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "No mem!");
        return NULL;
    }

    do
    {
        data_read = esp_http_client_read(client, &buffer[total_data_read], content_length);
        if (data_read < 0)
        {
            ESP_LOGE(TAG, "Data read error: %d %d", data_read, total_data_read);
            break;
        }
        total_data_read += data_read;
    } while(data_read != 0);

    ESP_LOGI(TAG, "%s", buffer);
    return buffer;
}

static esp_err_t telegram_io_set_headers(esp_http_client_handle_t client, telegram_io_header_t *headers)
{
    esp_err_t err = ESP_OK;
    int i = 0;
    
    while (headers[i].key)
    {
        err = esp_http_client_set_header(client, headers[i].key, headers[i].value);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Send message: esp_http_client_set_header failed err %d", err);
            break;
        }
        i++;
    }

    return err;
}

static esp_err_t telegram_io_prepare(esp_http_client_handle_t client, char *path, esp_http_client_method_t method, 
 telegram_io_header_t *headers)
{
    esp_err_t err = esp_http_client_set_url(client, path);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_http_client_set_url failed err %d", err);
        return err;
    }

    err = esp_http_client_set_method(client, method);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Send file: esp_http_client_set_method failed err %d", err);
        return err;
    }

    if (headers)
    {
        err = telegram_io_set_headers(client, headers);
        if (err != ESP_OK)
        {
            return err;
        }
    }

    return err;
}

static char *telegram_io_send_data(const char *path, uint32_t total_len, telegram_io_header_t *headers, 
    esp_http_client_method_t method, const char *post_field, void *ctx, telegram_io_send_file_cb_t cb)
{
    char *response = NULL;
    esp_err_t err;
    esp_http_client_handle_t client;
    uint32_t len_to_send = total_len;

    client = esp_http_client_init(&telegram_io_http_cfg);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to init http client");
        return NULL; 
    }

    err = telegram_io_prepare(client, (char *)path, method, headers);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "telegram_io_prepare failed err %d", err);
        esp_http_client_cleanup(client);
        return NULL;
    }

    if (post_field)
    {
        len_to_send += strlen(post_field);
    }

    err = esp_http_client_open(client, len_to_send);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Send file: esp_http_client_connect failed err %d", err);
        esp_http_client_cleanup(client);
        return NULL;
    }

    if (post_field)
    {
        int send_len = esp_http_client_write(client, post_field, strlen(post_field));
        if (send_len != strlen(post_field)) 
        {
            ESP_LOGW(TAG, "esp_http_client_write send_len != strlen(post_field)  %d", send_len);
        }

        if (send_len < 0)
        {
            ESP_LOGE(TAG, "Error during esp_http_client_write post_field");
            err = ESP_ERR_INVALID_SIZE;
        }
    }


    if ((err == ESP_OK) && (total_len != 0) && (cb != NULL))
    {
        int send_len;
        uint32_t max_size = 0;
        uint32_t chunk_size = 0;
        char *buffer = calloc(1, TELEGRAM_MAX_BUFFER);
        
        if (buffer == NULL)
        {
            ESP_LOGE(TAG, "No mem!");
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return NULL;
        }


        while (total_len)
        {
            max_size = min(total_len, TELEGRAM_MAX_BUFFER);
            chunk_size = cb(ctx, (uint8_t *)buffer, chunk_size);

            if (chunk_size > max_size)
            {
                err = ESP_ERR_INVALID_SIZE;
                ESP_LOGE(TAG, "chunk_size > max_size");
                break;
            }

            if (chunk_size == 0) 
            {
                break;
            }

            send_len = esp_http_client_write(client, buffer, chunk_size);
            if (send_len != chunk_size) 
            {
                ESP_LOGW(TAG, "esp_http_client_write send_len != chunk_size %d", send_len);
            }

            if (send_len < 0)
            {
                ESP_LOGE(TAG, "Error during esp_http_client_write");
                break;
            }


            total_len -= chunk_size;
        }

        free(buffer);
    }

    if (err == ESP_OK)
    {  
        esp_http_client_fetch_headers(client);
        response = telegram_io_read_all_content(client); 
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return response;
}

char *telegram_io_get(const char *path, telegram_io_header_t *headers)
{
    if (path == NULL)
    {
        ESP_LOGE(TAG, "Wrong arguments(get)");
        return NULL;
    }

    return telegram_io_send_data(path, 0, headers,  HTTP_METHOD_GET,  NULL, NULL, NULL);
}

void telegram_io_send(const char *path, const char *message, telegram_io_header_t *headers)
{
    char *buffer;
    if ((path == NULL) || (message == NULL))
    {
        ESP_LOGE(TAG, "Wrong arguments(send)");
        return;
    }

    ESP_LOGI(TAG, "Send message: %s", message);

    buffer = telegram_io_send_data(path, 0, headers,  HTTP_METHOD_POST,  (char *)message, NULL, NULL);
    free(buffer);
}

char *telegram_io_send_big(const char *path, uint32_t total_len, telegram_io_header_t *headers, 
    const char *post_field, void *ctx, telegram_io_send_file_cb_t cb)
{
    if ((path == NULL) || (cb == NULL) || (total_len == 0))
    {
        ESP_LOGE(TAG, "Wrong arguments(send_big)");
        return NULL;
    }

    return telegram_io_send_data(path, total_len, headers, HTTP_METHOD_POST, (char *)post_field, ctx, cb);
}