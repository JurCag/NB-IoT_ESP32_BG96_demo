#include "uart.h"
#include "esp_intr_alloc.h"
#include <soc/uart_struct.h>
#include <soc/uart_reg.h>

uint8_t cnt = 1;
#define LED 19

#define PWRKEY_PIN 23

#define BUF_SIZE 1024

uint8_t len;
uint8_t rxData0[512];
uint8_t* rxData2[128];

// Receive buffer to collect incoming data
uint8_t rx0Buf[256];
uart_isr_handle_t* handle_console;

volatile uint16_t rx0FifoLen, status, tx0FifoLen;
volatile uint16_t i = 0;

volatile bool data0Ready = false;

/*
const char keyStr[]= "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\
                        U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\
                        U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJ222222222\
                        U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJ333333333\
                        U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJ444444444\
                        U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJ555555555\
                        U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJ666666666";
*/

// IRAM_ATTR Forces code into IRAM instead of flash
// Otherwise it will be inside flash. Flash on the ESP32 is much slower than RAM access...
// See https://esp32.com/viewtopic.php?t=4978
static void IRAM_ATTR uart0IsrHandle(void *arg)
{
    status = UART0.int_st.val;             // read UART interrupt Status
    tx0FifoLen = UART0.status.rxfifo_cnt;
    rx0FifoLen = UART0.status.rxfifo_cnt; // read number of bytes in UART buffer

    while (rx0FifoLen>0)
    {
        rx0Buf[i++] = UART0.fifo.rw_byte; // read all bytes
        rx0FifoLen--;
    }
    i = 0;
    // after reading bytes from buffer clear UART interrupt status
    uart_clear_intr_status(UART_NUM_0, UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);

    // a test code or debug code to indicate UART receives successfully,
    // you can redirect received byte as echo also
    // uart_write_bytes(UART_NUM_1, (const char *)"RX Done", 7);
    data0Ready = true;
}

void app_main() {
    
    gpio_reset_pin(LED);
    // gpio_reset_pin(PWRKEY_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(PWRKEY_PIN, GPIO_MODE_OUTPUT);
    
    gpio_set_level(PWRKEY_PIN, 1);
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // gpio_set_level(PWRKEY_PIN, 0);
    // vTaskDelay(250 / portTICK_PERIOD_MS);
    // gpio_set_level(PWRKEY_PIN, 1);

    UART0_initConfig();
    UART0_initIO();
    UART0_installDriver();

    UART2_initConfig();
    UART2_initIO();
    UART2_installDriver();

    // uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    // uint8_t data[] = "Hello world  ";
    // uint8_t len = sizeof(data)/sizeof(uint8_t);

    uart_isr_free(UART_NUM_0);
    uart_isr_register(UART_NUM_0, uart0IsrHandle, NULL, ESP_INTR_FLAG_IRAM, handle_console);
    uart_enable_rx_intr(UART_NUM_0);
    
    while(1) 
    {
        // len = uart_read_bytes(UART_NUM_0, &rxData0, BUF_SIZE, 20 / portTICK_RATE_MS);
        // if (len> 3)
        //     uart_write_bytes(UART_NUM_2, (const char *) &rxData0, len);
        
        // if (len > 0)
            // uart_write_bytes(UART_NUM_2, (const char *) &data, len);
        
        // cnt++;
        gpio_set_level(LED, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        gpio_set_level(LED, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (data0Ready == true)
        {
            uart_write_bytes(UART_NUM_2, (const char *)rx0Buf, tx0FifoLen);
            data0Ready = false;
        }
        // uart_write_bytes(UART_NUM_2, (const char *)keyStr, sizeof(keyStr))/sizeof(char);


        // uart_write_bytes(UART_NUM_2, (const char *) &data, len);
        // if (cnt > 20)
        // {
        //     cnt = 0;
        // }

    }
}