#include "uart.h"
/* Functional Overview
1. Setting Communication Parameters - baud rate, data bits, stop bits, etc,
2. Setting Communication Pins - pins the other UART is connected to
3. Driver Installation - allocate ESP32’s resources for the UART driver
4. Running UART Communication - send / receive the data
5. Using Interrupts - trigger interrupts on specific communication events
6. Deleting Driver - release ESP32’s resources, if UART communication is not required anymore
*/

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

void UART2_initConfig(void)
{
    uart_param_config(UART_NUM_2, &uart_config);
}

// 2. Setting Communication Pins
void UART0_initIO(void)
{
    uart_set_pin(UART_NUM_0, UART0_TX_PIN, UART0_RX_PIN, UARTx_RTS_PIN, UARTx_CTS_PIN);
}

void UART2_initIO(void)
{
    uart_set_pin(UART_NUM_2, UART2_TX_PIN, UART2_RX_PIN, UARTx_RTS_PIN, UARTx_CTS_PIN);
}

// 3. Driver Installation
void UART0_installDriver(void)
{
    uart_driver_install(UART_NUM_0, 2048, 2048, 0, NULL, 0);
}

void UART2_installDriver(void)
{
    uart_driver_install(UART_NUM_2, 2048, 2048, 0, NULL, 0);
}