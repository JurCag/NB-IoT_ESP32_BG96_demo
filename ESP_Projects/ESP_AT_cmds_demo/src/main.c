#include "uart.h"
#include <soc/uart_struct.h>
#include <soc/uart_reg.h>
#include <string.h>

#define LED 19
#define PWRKEY_PIN 23

uint8_t ledState = 0;


/* Fcn declarations */
static void uart0RxDataParser(void);

uint8_t uart0RxData[BUFFER_SIZE];
uint8_t uart2RxData[BUFFER_SIZE];
size_t* uart0RxLen;
int len0;
int len2;

void taskUart0(void *pvParameters)
{
    while (1) 
    {
        // Read data from the UART
        len0 = uart_read_bytes(UART_NUM_0, uart0RxData, BUFFER_SIZE, 50 / portTICK_RATE_MS);
        // Write data back to the UART
        if (len0 > 0)
        {
            uart_write_bytes(UART_NUM_2, (const char *)uart0RxData, len0);
        }
    }
}

void taskUart2(void *pvParameters)
{
    while (1) 
    {
        // Read data from the UART
        len2 = uart_read_bytes(UART_NUM_2, uart2RxData, BUFFER_SIZE, 50 / portTICK_RATE_MS);
        // Write data back to the UART
        if (len2 > 0)
        {
            uart_write_bytes(UART_NUM_0, (const char *) uart2RxData, len2);
        }
    }
}

void taskUart0Parser(void *pvParameters)
{
    while(1)
    {
        vTaskDelay(1);
        if(len0 > 0)
        {
            uart0RxDataParser();
        }
    }
}


void app_main() 
{
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

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


    xTaskCreate(
                taskUart0,          /* Task function. */
                "taskUart0",        /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY,   /* priority of the task */
                NULL                /* Task handle to keep track of created task */
                );

    xTaskCreate(
                taskUart2,          /* Task function. */
                "taskUart2",        /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY,   /* priority of the task */
                NULL                /* Task handle to keep track of created task */
                );

    xTaskCreate(
                taskUart0Parser,    /* Task function. */
                "taskUart0Parser",  /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY + 1 ,   /* priority of the task */
                NULL                /* Task handle to keep track of created task */
                );
}

static void uart0RxDataParser(void)
{
    if (strncmp((const char *)"power on", (const char *)uart0RxData, 8) == 0)
    {
        gpio_set_level(PWRKEY_PIN, 1);
        vTaskDelay(750 / portTICK_PERIOD_MS);
        gpio_set_level(PWRKEY_PIN, 0);
    }
}
