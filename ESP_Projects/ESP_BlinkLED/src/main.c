#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BLINK_GPIO  GPIO_NUM_19
static uint8_t ledState = 0;

// If connected more COM devices
// check in platformio.ini whether correct upload_port is configured

void app_main() 
{
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while (1) {

        /* Toggle the LED state */
        ledState = !ledState;
        gpio_set_level(BLINK_GPIO, ledState);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}