#include "uart.h"
#include "esp_intr_alloc.h"
#include <soc/uart_struct.h>
#include <soc/uart_reg.h>
#include <string.h>

#define LED 19
#define PWRKEY_PIN 23

volatile uint16_t rx0FifoLen, status, tx0FifoLen;
volatile uint16_t i = 0;
volatile bool data0Ready = false;

volatile uint16_t rx2FifoLen, tx2FifoLen;
volatile uint16_t j = 0;
volatile bool data2Ready = false;

uint8_t ledState = 0;

// Receive buffer to collect incoming data
volatile uint8_t rx0Buf[256];
volatile uint8_t rx2Buf[256];
uart_isr_handle_t* uart0ISRHandle;
uart_isr_handle_t* uart2ISRHandle;

/* Fcn declarations */
// static void IRAM_ATTR ISR_uart0(void *arg);
static void uart0RxDataParser(void);

TaskHandle_t taskUart0Handle;
TaskHandle_t taskUart2Handle;

static void ISR_uart0(void *arg) //IRAM_ATTR
{
    status = UART0.int_st.val;             // read UART interrupt Status
    tx0FifoLen = UART0.status.rxfifo_cnt;
    rx0FifoLen = UART0.status.rxfifo_cnt; // read number of bytes in UART buffer

    while (rx0FifoLen > 0)
    {
        rx0Buf[i++] = UART0.fifo.rw_byte; // read all bytes
        rx0FifoLen--;
    }
    i = 0;
    uart_clear_intr_status(UART_NUM_0, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);

    ledState = !ledState;
    gpio_set_level(LED, ledState);
    data0Ready = true;
}
volatile uint8_t cnt;
static void ISR_uart2(void *arg) //IRAM_ATTR
{
    uart_clear_intr_status(UART_NUM_2, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
    tx2FifoLen = UART2.status.rxfifo_cnt;
    rx2FifoLen = UART2.status.rxfifo_cnt; // read number of bytes in UART buffer

    while (rx2FifoLen > 0)
    {
        rx2Buf[i++] = UART0.fifo.rw_byte; // read all bytes
        rx2FifoLen--;
    }
    j = 0;
    // uart_clear_intr_status(UART_NUM_2, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
    while (UART2.int_st.rxfifo_tout == 1)
    {
        uart_clear_intr_status(UART_NUM_2, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
        if(cnt++>3)
        {
            cnt = 0;
            break;
        }
    }
    
    ledState = !ledState;
    gpio_set_level(LED, ledState);
    data2Ready = true;
}

void taskUart0(void *pvParameters)
{
    while (1)
    {
        if (data0Ready == true)
        {
            uart0RxDataParser();
            uart_write_bytes(UART_NUM_2, (const char *)rx0Buf, tx0FifoLen);

            data0Ready = false;
        }
    }
}

void taskUart2(void *pvParameters)
{
    while(1) 
    {
        if (data2Ready == true)
        {
            // uart_write_bytes(UART_NUM_0, (const char *)rx2Buf, tx2FifoLen);
            gpio_set_level(LED, 0);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            gpio_set_level(LED, 1);
            vTaskDelay(250 / portTICK_PERIOD_MS);

            data2Ready = false;
        }
    }
}

void app_main() 
{
    // disableCore0WDT();
    // disableCore1WDT();

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

    /* UART0 config interrupts */
    uart_isr_free(UART_NUM_0);
    uart_isr_register(UART_NUM_0, ISR_uart0, NULL, ESP_INTR_FLAG_LEVEL1, uart0ISRHandle);
    uart_enable_rx_intr(UART_NUM_0);

    /* UART2 config interrupts */
    uart_isr_free(UART_NUM_2);
    uart_isr_register(UART_NUM_2, ISR_uart2, NULL, ESP_INTR_FLAG_LEVEL1, uart2ISRHandle);
    uart_enable_rx_intr(UART_NUM_2);

    // xTaskCreatePinnedToCore(
    //                 taskUart0,        /* Task function. */
    //                 "taskUart0",      /* name of task. */
    //                 4096,             /* Stack size of task */
    //                 NULL,             /* parameter of the task */
    //                 tskIDLE_PRIORITY, /* priority of the task */
    //                 &taskUart0Handle, /* Task handle to keep track of created task */
    //                 0);

    // xTaskCreatePinnedToCore(
    //                 taskUart2,        /* Task function. */
    //                 "taskUart2",      /* name of task. */
    //                 4096,             /* Stack size of task */
    //                 NULL,             /* parameter of the task */
    //                 tskIDLE_PRIORITY, /* priority of the task */
    //                 &taskUart2Handle, /* Task handle to keep track of created task */
    //                 0);
}



static void uart0RxDataParser(void)
{
    if (strncmp((const char *)"power on", (const char *)rx0Buf, 8) == 0)
    {
        // ledState = !ledState;
        // gpio_set_level(LED, ledState);

        gpio_set_level(PWRKEY_PIN, 1);
        vTaskDelay(750 / portTICK_PERIOD_MS);
        gpio_set_level(PWRKEY_PIN, 0);
    }
}
