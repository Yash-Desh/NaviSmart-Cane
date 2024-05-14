#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include <stdbool.h>
#include <ultrasonic.h>
#include <esp_err.h>

#include "driver/adc.h"
#include "driver/ledc.h"


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

    uart_driver_install(UART_NUM_0, RX_BUF_SIZE*2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


//for UART2
// For communication between GPS & esp32
void init2(void)
{
    const uart_config_t uart_config_2 = {
        .baud_rate = 9600,                   // Default GPS Baud Rate
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_driver_install(UART_NUM_2, RX_BUF_SIZE*2, 0,0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config_2);
    uart_set_pin(UART_NUM_2, TXD_PIN2, RXD_PIN2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static void rx_task(void *arg)
{
    uint8_t* data = (uint8_t*)malloc(RX_BUF_SIZE+1);
    while(1)
    {
        const int rxbytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 1000/portTICK_RATE_MS);
        if(rxbytes>0)
        {
            data[rxbytes] = 0;
            uart_write_bytes(UART_NUM_0,(const char*)data, rxbytes); 
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    free(data);
}

void ultrasonic_test(void *pvParameters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };

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
            printf("Distance: %0.04f cm\n", distance*100);
            float dist = distance * 100;
            if (dist>0 && dist < 10)
            {
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 200);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
            }

            else if( dist >=10 && dist < 30)
            {
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 100);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(50));
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if(dist >=30 && dist < 75)
            {

                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 50);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(150));
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                vTaskDelay(pdMS_TO_TICKS(150));
                
            }
            else if (dist >=75 && dist < 100)
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
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}