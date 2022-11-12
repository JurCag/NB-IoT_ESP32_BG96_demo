
#include <driver/gpio.h>
#include <driver/uart.h>

#define UART0_TX_PIN    (GPIO_NUM_23) // !!!JUST CHANGE PIN NUMBERS TO ARBITARY GPIO uart_set_pin solves the rest!!!
#define UART0_RX_PIN    (GPIO_NUM_22) // !!!JUST CHANGE PIN NUMBERS TO ARBITARY GPIO uart_set_pin solves the rest!!!
#define UARTx_RTS_PIN   (UART_PIN_NO_CHANGE)
#define UARTx_CTS_PIN   (UART_PIN_NO_CHANGE)

#define BUFFER_SIZE     (1024)

uint8_t uart0RxData[BUFFER_SIZE];
int len0;

// 1. Setting Communication Parameters
uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
};

void UART0_initConfig(void)
{
    uart_param_config(UART_NUM_0, &uart_config);
}

// 2. Setting Communication Pins
void UART0_initIO(void)
{
    uart_set_pin(UART_NUM_0, UART0_TX_PIN, UART0_RX_PIN, UARTx_RTS_PIN, UARTx_CTS_PIN);
}

// 3. Driver Installation
void UART0_installDriver(void)
{
    uart_driver_install(UART_NUM_0, BUFFER_SIZE * 2, BUFFER_SIZE * 2, 0, NULL, 0);
}

void UART0_writeBytes(const char* bytes, size_t len)
{
    uart_write_bytes(UART_NUM_0, bytes, len);
}

void taskUart0(void *pvParameters)
{
    while (1) 
    {
        // Read data from the UART
        len0 = uart_read_bytes(UART_NUM_0, uart0RxData, BUFFER_SIZE, 50 / portTICK_RATE_MS);
        // Write data back to the UART
        if (len0 > 0)
        {
            uart_write_bytes(UART_NUM_0, (const char *)uart0RxData, len0);
        }
    }
}


void app_main() 
{
    /* UART0 communicates with PC */
    UART0_initConfig();
    UART0_initIO();
    UART0_installDriver();

    xTaskCreate(
                taskUart0,          /* Task function. */
                "taskUart0",        /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY,   /* priority of the task */
                NULL                /* Task handle to keep track of created task */
                );
}