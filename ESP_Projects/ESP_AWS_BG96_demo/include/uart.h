#ifndef __UART_H__
#define __UART_H__

#include <driver/gpio.h>
#include <driver/uart.h>
#include <string.h>

#define UART0_TX_PIN    (GPIO_NUM_18)
#define UART0_RX_PIN    (GPIO_NUM_5)
#define UART2_TX_PIN    (GPIO_NUM_17)
#define UART2_RX_PIN    (GPIO_NUM_16)

#define UARTx_RTS_PIN   (UART_PIN_NO_CHANGE)
#define UARTx_CTS_PIN   (UART_PIN_NO_CHANGE)

#define BUFFER_SIZE     (256)

// UART 0
void UART0_initConfig(void);
void UART0_initIO(void);
void UART0_installDriver(void);
void UART0_writeBytes(const char* bytes);

// UART 2
void UART2_initConfig(void);
void UART2_initIO(void);
void UART2_installDriver(void);
void UART2_writeBytes(const char* bytes);


#endif // __UART_H__
