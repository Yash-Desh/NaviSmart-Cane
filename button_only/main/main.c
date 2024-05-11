// Author : Yash Deshpande
// Date : 01-02-2024
// Tutor : Yuri R YouTube

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    // Initializing GPIO22 as an input with a pull up
   gpio_set_direction(GPIO_NUM_22, GPIO_MODE_INPUT);
   gpio_set_pull_mode(GPIO_NUM_22, GPIO_PULLUP_ONLY);


    // Initializing GPIO26 as the ouput
    gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);

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
            printf("LED On\n");
        }
        vTaskDelay(10);
    }

}