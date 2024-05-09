


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_http_client.h"
#include "connect_wifi.h"

static const char *TAG = "HTTP_CLIENT";

// Your Account SID and Auth Token from twilio.com/console
char account_sid[] = "ACd75d1e318b372f307ec0ebc1553f7a98";
char auth_token[] = "c7215912d183bd625256a686f16cc277";
char recipient_num[] = "+919167791254";
char sender_num[] = "+18482130985";



char message[] = "Hello This is a test message";

void twilio_send_sms(void *pvParameters)
{

    char twilio_url[200];
    snprintf(twilio_url,
             sizeof(twilio_url),
             "%s%s%s",
             "https://api.twilio.com/2010-04-01/Accounts/",
             account_sid,
             "/Messages");

    char post_data[200];

    snprintf(post_data,
             sizeof(post_data),
             "%s%s%s%s%s%s",
             "To=",
             recipient_num,
             "&From=",
             sender_num,
             "&Body=",
             message);

    esp_http_client_config_t config = {
        .url = twilio_url,
        .method = HTTP_METHOD_POST,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_username(client, account_sid);
    esp_http_client_set_password(client, auth_token);

    ESP_LOGI(TAG, "post = %s", post_data);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 201)
        {
            ESP_LOGI(TAG, "Message sent Successfully");
        }
        else
        {
            ESP_LOGI(TAG, "Message sent Failed");
        }
    }
    else
    {
        ESP_LOGI(TAG, "Message sent Failed");
    }
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	connect_wifi();
	if (wifi_connect_status)
	{
		xTaskCreate(&twilio_send_sms, "twilio_send_sms", 8192, NULL, 6, NULL);
	}
}
