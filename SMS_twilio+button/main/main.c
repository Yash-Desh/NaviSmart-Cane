// Author : Yash Deshpande
// Date : 01-02-2024
// Tutor : Yuri R YouTube

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
char account_sid[] = "";
char auth_token[] = "";
char recipient_num[] = "+919167791254";
char sender_num[] = "+12177591518";



char message[] = " http://maps.google.co.uk/maps?q=51.917168,-0.227051";

void twilio_send_sms()
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
            printf("after mssg sent successi");
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
    printf("out of twiiosmsfunc");
    //vTaskDelete(NULL);
}


void button_press(){
    

    while(true)
    {
        if (gpio_get_level(GPIO_NUM_22))
        {
            gpio_set_level(GPIO_NUM_26, 0);
          
            printf("LED Off\n");
        }
        else
        {
            gpio_set_level(GPIO_NUM_26, 1);
              twilio_send_sms();
            printf("LED On\n");
        }
        vTaskDelay(10);
    }
}
void app_main(void)
{

      gpio_set_direction(GPIO_NUM_22, GPIO_MODE_INPUT);
   gpio_set_pull_mode(GPIO_NUM_22, GPIO_PULLUP_ONLY);


    // Initializing GPIO26 as the ouput
    gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);
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
		xTaskCreate(&button_press, "button_press", 8192, NULL, 6, NULL);
	}
}


