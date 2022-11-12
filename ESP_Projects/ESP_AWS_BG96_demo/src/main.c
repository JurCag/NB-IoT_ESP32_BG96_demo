#include "main.h"

void app_main() {

    gpio_reset_pin(PWRKEY_PIN);
    gpio_set_direction(PWRKEY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PWRKEY_PIN, 0);

    /* UART0 communicates with PC */
    UART0_initConfig();
    UART0_initIO();
    UART0_installDriver();

    /* UART2 forwards data to GSM modem */
    UART2_initConfig();
    UART2_initIO();
    UART2_installDriver();

    char str[40];
    sprintf(str, "\r\nConfig FreeRTOS freq = %d Hz\r\n", CONFIG_FREERTOS_HZ);
    UART0_writeBytes(str);

    /* Initialize FreeRTOS components */
    createRxQueue();
    createAtPacketsTxQueue();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    /* Create FreeRTOS tasks */
    createTaskRx();
    createTaskPowerUpModem(PWRKEY_PIN);

}

