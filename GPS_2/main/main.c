#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
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
    const char *str = "$GPGGA";
    char *p = NULL;
    char str1[100];
    int j = 0;
    uint8_t* data = (uint8_t*)malloc(RX_BUF_SIZE+1);
    char *str2 = (char*)malloc(sizeof(str1));
    while(1)
    {
        const int rxbytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 1000/portTICK_RATE_MS);
        
        if(rxbytes>0)
        {
            p = strstr((const char *)data, str);
            if(p)
            {
                for(int i=0; p[i] != '\n'; i++)
                {
                    str2[j] = p[i];
                    j++;
                }
                str2[j+1] = '\n';
                const int len = j+2;
                data[rxbytes] = 0;
                uart_write_bytes(UART_NUM_0,(const char*)str2, len);
            }
            
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    free(data);
    free(str2);
}


void app_main(void)
{
    init();
    init2();
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}