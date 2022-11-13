#ifndef __BG96_H__
#define __BG96_H__

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include "AT_cmds.h"
#include "uart.h"
#include "secrets.h"
#include "BG96_ssl.h"
#include "BG96_tcpip.h"
#include "BG96_mqtt.h"



#define UART_ESPxPC         (UART_NUM_0)
#define UART_ESPxBG96       (UART_NUM_2)

#define MS_TO_TICKS(ms)     (ms/portTICK_PERIOD_MS)
#define TASK_DELAY_MS(ms)   (vTaskDelay(MS_TO_TICKS(ms)))

#define BG96_HOLD_POWER_UP_PIN_MS   (750) 

#define RESEND_ATTEMPTS     (4)

/* Uncomment to see intern communication (ESP32 <==> BG96) */
#define DUMP_INTER_COMM

/* Uncomment to see info */
#define PRINT_INFO

/* Typedefs */
typedef void (*BG96_txPacketCB_t)(char* packet); 

typedef enum
{
    TASK_IDLE   = 0,
    TASK_RUN    = 1
} TaskState_t;

/* Variables */
ContextID_t contextID;
char contextIdStr[8];
SslContextID_t SSL_ctxID;
char sslCtxIdStr[8];
MqttSocketIdentifier_t client_idx;
char clientIdxStr[8];
QueueHandle_t rxDataQueue;

/* Functions */
void BG96_txAtCmd(char* packet);
void queueAtPacket(AtCmd_t* cmd, AtCmdType_t type);
void prepareArg(char** paramsArr, uint8_t numOfParams, char* arg);

/* FreeRTOS */
void createTaskRx(void);
void createTaskTx(void);
void createTaskPowerUpModem(gpio_num_t pwrKeyPin);
void createTaskFeedTxQueue(void);

void createRxQueue(void);
void createAtPacketsTxQueue(void);

void dumpInterComm(char* msg);
void printInfo(char* msg);

// void createTaskTest(void);

#endif // __BG96_H__
