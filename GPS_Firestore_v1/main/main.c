// Author : Yash Deshpande
// Date : 08-05-2024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_http_server.h"  // Not being used
#include "my_data.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/uart.h"



static const int RX_BUF_SIZE = 2024;


// for UART 0
// Displays data to the serial monitor
#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)

#define TXD_PIN2 (GPIO_NUM_17)
#define RXD_PIN2 (GPIO_NUM_16)

// for UART0
// For communication between PC & esp32
// Displays data to the serial monitor
#define TXD_PIN (GPIO_NUM_1)
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


double lat, longi;
char latitude[100] = "empty";
char longitude[100] = "empty";

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
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
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
        const int rxbytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);

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


// WiFi
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting ... \n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected ... \n");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection ... \n");
        break;
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        break;
    default:
        break;
    }
}

void wifi_connection()
{
    // 1 - Wi-Fi/LwIP Init Phase
    esp_netif_init();                    // TCP/IP initiation 					s1.1
    esp_event_loop_create_default();     // event loop 			                s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station 	                    s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); // 					                    s1.4
    
    // 2 - Wi-Fi Configuration Phase
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID,
            .password = PASS}};
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();

}


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

    char json_string[1024];
    snprintf(json_string, 1024,
         "{\"fields\":{\"Memory\":{\"doubleValue\":%f},\"Name\":{\"doubleValue\":\"%f\"}}}",
         lat, longi);

    // char s1[] = "{\"fields\":{\"latitude\":{\"stringValue\":\"";
    // char s2[] = "\"},\"longitude\":{\"stringValue\":\"";
    // char s3[] = "\"}}}";
    // strcat(s1,latitude);
    // strcat(s1,s2);
    // strcat(s1, longitude);
    // strcat(s1, s3);

    char *post_data = json_string;
    // char *post_data = "{\"fields\":{\"Memory\":{\"doubleValue\":\"2\"},\"Name\":{\"stringValue\":\"Old ESP32\"}}}";
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


void app_main(void)
{
    init();
    init2();
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);

    // Non-volatile Storage
    nvs_flash_init();

    // 
    wifi_connection();

    // REST GET data
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated.................\n\n");
    
    post_rest_function();

}