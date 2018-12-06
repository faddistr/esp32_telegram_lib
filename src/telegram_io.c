#include <string.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include "telegram_io.h"

#define TELEGRAM_MAX_BUFFER 4095U

static const char *TAG="telegram_io";

static esp_err_t telegram_http_event_handler(esp_http_client_event_t *evt);

static const esp_http_client_config_t telegram_io_http_cfg = 
{
        .event_handler = telegram_http_event_handler,
        .timeout_ms = 5000,
        .url = "http://example.org",
};

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
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
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



char *telegram_io_get(const char *path, const char *post_data)
{
    int data_read = 0;
    int total_data_read = 0; 
	int content_length;
	esp_err_t err;
	esp_http_client_handle_t client;
    char *buffer = NULL;

    if (path == NULL)
    {
        return NULL;
    }

	client = esp_http_client_init(&telegram_io_http_cfg);
	if (client  == NULL)
	{
		ESP_LOGE(TAG, "Failed to initialise HTTP connection");
		return NULL;
	}
	
    err = esp_http_client_set_url(client, path);
    if (err  != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection esp_http_client_set_url:%d", err);
        esp_http_client_cleanup(client);
        return NULL;
    }
    
    err = esp_http_client_set_method(client, (post_data == NULL)?HTTP_METHOD_GET:HTTP_METHOD_POST);
    if (err  != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection esp_http_client_set_method:%d", err);
        esp_http_client_cleanup(client);
        return NULL;
    }

    if (post_data != NULL)
    {
        err = esp_http_client_set_post_field(client, post_data, strlen(post_data));
        if (err  != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to initialise HTTP connection esp_http_client_set_post_field:%d", err);
            esp_http_client_cleanup(client);
            return NULL;
        }
    }
    
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection esp_http_client_open:%d", err);
        esp_http_client_cleanup(client);
        return NULL;   
    }

    esp_http_client_fetch_headers(client);

    content_length =  esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
               content_length);
    if ((content_length < TELEGRAM_MAX_BUFFER) && (content_length > 0))
    {
        buffer = calloc(1, content_length + 1);
    }
    else
    {
        ESP_LOGE(TAG, "Content length wrong:%d", content_length);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return NULL;   
    }

    while (1) {
        data_read = esp_http_client_read(client, &buffer[total_data_read], content_length);
        total_data_read += data_read;
        if (data_read < 0)
        {
        	break;
        }

        if ((data_read == 0)) {
            ESP_LOGI(TAG, "Connection closed,all data received total_data_read = %d content_length = %d data_read = %d",
            	total_data_read, content_length, data_read);
            break;
        }
    }
    
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return buffer;
}

void telegram_io_send(const char *path, const char *message, telegram_io_header_t *header)
{
    esp_err_t err;
    esp_http_client_handle_t client;

    if ((path == NULL) || (message == NULL))
    {
        ESP_LOGE(TAG, "Wrong arguments(send)");
        return;
    }

    client = esp_http_client_init(&telegram_io_http_cfg);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to init http client");
        return; 
    }

    ESP_LOGI(TAG, "Send message: %s", message);

    do
    {
        int i;

        err = esp_http_client_set_url(client, path);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_http_client_set_url failed err %d", err);
            break;
        }

        i = 0;
        while (header[i].key)
        {
            err = esp_http_client_set_header(client, header[i].key, header[i].value);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Send message: esp_http_client_set_header failed err %d", err);
                break;
            }
            i++;
        }

        if (err != ESP_OK)
        {
            break;
        }
        

        err = esp_http_client_set_method(client, HTTP_METHOD_POST);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Send message: esp_http_client_set_method failed err %d", err);
            break;
        }

        err = esp_http_client_set_post_field(client, message, strlen(message));
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Send message: esp_http_client_set_post_field failed err %d", err);
            break;
        }

        err= esp_http_client_perform(client);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Send message: esp_http_client_perform failed err %d", err);
            break;
        }
    } while (0);

    esp_http_client_cleanup(client);
}