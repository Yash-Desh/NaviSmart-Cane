// Author : Yash Deshpande


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"



#include "esp_system.h"

#include "esp_http_client.h"
#include "connect_wifi.h"





#include "esp_system.h"
#include "esp_log.h"

#include "driver/uart.h"

#include <stdbool.h>
#include <ultrasonic.h>
#include <esp_err.h>

#include "driver/adc.h"
#include "driver/ledc.h"





#include <sys/param.h>


#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_http_server.h" // Not being used
// #include "my_data.h"
#include "esp_system.h"

#include "driver/uart.h"

static const int RX_BUF_SIZE_Firestore = 2024;

double lat, longi;
char latitude[100] = "empty";
char longitude[100] = "empty";


static const char *TAG = "HTTP_CLIENT";

// Your Account SID and Auth Token from twilio.com/console
char account_sid[] = "Your_Account_sid";
char auth_token[] = "Your_auth_token";
char recipient_num[] = "+Your_recipient_num";
char sender_num[] = "+Your_sender_num";

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
    // vTaskDelete(NULL);
}

void button_press()
{

    while (true)
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



static const int RX_BUF_SIZE = 2024;

#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)

#define TXD_PIN2 (GPIO_NUM_17)
#define RXD_PIN2 (GPIO_NUM_16)

#define MAX_DISTANCE_CM 500 // 5m max

#define TRIGGER_GPIO 5
#define ECHO_GPIO 18

#define SAMPLE_CNT 32
static const adc1_channel_t adc_channel = ADC_CHANNEL_4;
#define LEDC_GPIO 27
static ledc_channel_config_t ledc_channel;

static void init_hw(void)
{
    adc1_config_width(ADC_WIDTH_BIT_10);
    adc1_config_channel_atten(adc_channel, ADC_ATTEN_DB_11);
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 1000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ledc_timer_config(&ledc_timer);
    ledc_channel.channel = LEDC_CHANNEL_0;
    ledc_channel.duty = 0;
    ledc_channel.gpio_num = LEDC_GPIO;
    ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel.hpoint = 0;
    ledc_channel.timer_sel = LEDC_TIMER_0;
    ledc_channel_config(&ledc_channel);
}

// for UART0
// For communication between PC & esp32
void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

// for UART2
//  For communication between GPS & esp32
void init2(void)
{
    const uart_config_t uart_config_2 = {
        .baud_rate = 9600, // Default GPS Baud Rate
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config_2);
    uart_set_pin(UART_NUM_2, TXD_PIN2, RXD_PIN2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static void rx_task(void *arg)
{
    const char *str = "$GPGGA";
    char *p = NULL;
    char str1[100];
    int j = 0;
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE_Firestore + 1);
    char *str2 = (char *)malloc(sizeof(str1));

    int comma_count = 0;

    char *lat_ptr;
    char *longi_ptr;

    int lat_start_pos = 0;
    int lat_len = 0;

    int long_start_pos = 0;
    int long_len = 0;

    while (1)
    {
        const int rxbytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE_Firestore, 1000 / portTICK_RATE_MS);

        if (rxbytes > 0)
        {
            p = strstr((const char *)data, str);
            if (p)
            {

                for (int i = 0; p[i] != '\n'; i++)
                {
                    str2[j] = p[i];
                    j++;
                }
                str2[j + 1] = '\n';
                const int len = j + 2;
                data[rxbytes] = 0;
                uart_write_bytes(UART_NUM_0, (const char *)str2, len);

                // logic to extract the latitude & longitude

                for (int i = 0; str2[i] != '\n'; i++)
                {
                    if (str2[i] == ',')
                    {
                        comma_count++;
                        if (comma_count == 2 && i != 0)
                        {
                            lat_start_pos = i + 1;
                        }
                        if (comma_count == 3 && i != 0)
                        {
                            lat_len = i - lat_start_pos;
                        }
                        if (comma_count == 4 && i != 0)
                        {
                            long_start_pos = i + 1;
                        }
                        if (comma_count == 5 && i != 0)
                        {
                            long_len = i - long_start_pos;
                        }
                    }
                }

                strncpy(latitude, p + (lat_start_pos), lat_len);
                strncpy(longitude, p + (long_start_pos), long_len);
                printf("\nlatitude = %s \nlongitude = %s\n\n", latitude, longitude);

                lat = strtod(latitude, &lat_ptr);
                longi = strtod(longitude, &longi_ptr);
                printf("\nlatitude = %f \nlongitude = %f\n\n", lat, longi);
                
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    free(data);
    free(str2);
}

extern const uint8_t certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t certificate_pem_end[] asm("_binary_certificate_pem_end");


// Client
esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("Client HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}

void post_rest_function()
{
    esp_http_client_config_t config_get = {
        .url = "https://firestore.googleapis.com/v1/projects/smart-navi-stick/databases/(default)/documents/test",
        .method = HTTP_METHOD_POST,
        .cert_pem = (const char *)certificate_pem_start,
        .event_handler = client_event_get_handler};

    esp_http_client_handle_t client = esp_http_client_init(&config_get);

    // char json_string[1024];
    // snprintf(json_string, 1024,
    //      "{\"fields\":{\"Memory\":{\"doubleValue\":%f},\"Name\":{\"doubleValue\":\"%f\"}}}",
    //      lat, longi);

    char s1[100] = "{\"fields\":{\"latitude\":{\"stringValue\":\"";
    char s2[] = "\"},\"longitude\":{\"stringValue\":\"";
    char s3[] = "\"}}}";
    // char lat1[] = "234.65";
    // char long1[] = "344.67";
    strcat(s1, latitude);
    strcat(s1, s2);
    strcat(s1, longitude);
    strcat(s1, s3);

    // Function Call

    char *post_data;
    post_data = (char*)malloc(strlen(s1));
 
    strcpy(post_data, s1);

    memcpy(post_data, s1, strlen(s1));
    // post_data = copyString(s1);
    // char *post_data = "{\"fields\":{\"Memory\":{\"doubleValue\":\"2\"},\"Name\":{\"stringValue\":\"Old ESP32\"}}}";
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void firestore_post_task(void *arg)
{
    while(1)
    {
        post_rest_function();
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void ultrasonic_test(void *pvParameters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO};

    ultrasonic_init(&sensor);

    while (true)
    {
        float distance;
        esp_err_t res = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK)
        {
            printf("Error %d: ", res);
            switch (res)
            {
            case ESP_ERR_ULTRASONIC_PING:
                printf("Cannot ping (device is in invalid state)\n");
                break;
            case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                printf("Ping timeout (no device found)\n");
                break;
            case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                printf("Echo timeout (i.e. distance too big)\n");
                break;
            default:
                printf("%s\n", esp_err_to_name(res));
            }
        }
        else
        {
            printf("Distance: %0.04f cm\n", distance * 100);
            float dist = distance * 100;
            if (dist > 0 && dist < 10)
            {
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 200);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
            }

            else if (dist >= 10 && dist < 30)
            {
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 100);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(50));
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (dist >= 30 && dist < 75)
            {

                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 50);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(150));
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            else if (dist >= 75 && dist < 100)
            {
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 10);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(500));
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            else
            {
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}



void app_main(void)
{
    init();
    init2();
    init_hw();
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
        // xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
        // xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
        xTaskCreate(firestore_post_task, "firestore_post", 4096 * 2, NULL, 5, NULL);
    
    }

    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

}
