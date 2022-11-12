#ifndef __UART_H__
#define __UART_H__

#include <driver/gpio.h>
#include <driver/uart.h>

#define UART0_TX_PIN    (GPIO_NUM_4)
#define UART0_RX_PIN    (GPIO_NUM_2)
#define UART2_TX_PIN    (GPIO_NUM_17)
#define UART2_RX_PIN    (GPIO_NUM_16)

#define UARTx_RTS_PIN   (UART_PIN_NO_CHANGE)
#define UARTx_CTS_PIN   (UART_PIN_NO_CHANGE)

#define BUFFER_SIZE     (1024)

// UART 0
void UART0_initConfig(void);
void UART0_initIO(void);
void UART0_installDriver(void);

// UART 2
void UART2_initConfig(void);
void UART2_initIO(void);
void UART2_installDriver(void);


#endif // __UART_H__
