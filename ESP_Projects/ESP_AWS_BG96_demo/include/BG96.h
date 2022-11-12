#ifndef __BG96_H__
#define __BG96_H__

#include <stdint.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include "AT_cmds.h"
#include "uart.h"

#define UART_ESPxPC         (UART_NUM_0)
#define UART_ESPxBG96       (UART_NUM_2)

#define MS_TO_TICKS(ms)     (ms/portTICK_PERIOD_MS)
#define TASK_DELAY_MS(ms)   (vTaskDelay(MS_TO_TICKS(ms)))

#define BG96_HOLD_POWER_UP_PIN_MS   (750) 

/* Typedefs */
typedef void (*BG96_txPacketCB_t)(char* packet); 

typedef enum
{
    TASK_IDLE   = 0,
    TASK_RUN    = 1
} TaskState_t;

typedef struct
{
    AtCmd_t* atCmd;
    AtCmdType_t atCmdType;
} BG96_AtPacket_t;

typedef struct
{
    char b[BUFFER_SIZE];
} RxData_t;

/* Variables */

/* Functions */
void BG96_txPacket(char* packet);

/* FreeRTOS */
void createTaskRx(void);
void createTaskTx(void);
void createTaskFeedTxQueue(void);
void createTaskPowerUpModem(gpio_num_t pwrKeyPin);

void createRxQueue(void);
void createAtPacketsTxQueue(void);

// void createTaskTest(void);

#endif // __BG96_H__
