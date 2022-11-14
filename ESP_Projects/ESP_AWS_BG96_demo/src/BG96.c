#include "BG96.h"

QueueHandle_t rxDataQueue = NULL;


/* Local variables */
static BG96_AtPacket_t deqdAtPacket;
static QueueHandle_t atPacketsTxQueue = NULL;
static TaskHandle_t taskRxHandle = NULL;
static TaskHandle_t taskTxHandle = NULL;
static TaskHandle_t taskPowerUpModemHandle = NULL;
static TaskHandle_t taskFeedTxQueueHandle = NULL;

// Common indexes for this connection/application (BG96 can open more connection to the GSM network)
ContextID_t contextID = CONTEXT_ID_1;
char contextIdStr[8];
SslContextID_t SSL_ctxID = SSL_CTX_ID_0;
char sslCtxIdStr[8];
MqttSocketIdentifier_t client_idx = CLIENT_IDX_0;
char clientIdxStr[8];


/* Local FreeRTOS tasks */
static void taskFeedTxQueue(void* pvParameters);
static void taskRx(void* pvParameters);
static void taskTx(void *pvParameters);
static void taskPowerUpModem(void *pvParameters);
// static void taskTest(void *pvParameters);

/* Local functions */
static void powerUpModem(gpio_num_t pwrKeypin);
static void swPowerDownModem(void);
static void queueRxData(RxData_t rxData);
static void initCommonConnParams(void);

static uint8_t responseParser(void);
static void BG96_atCmdFamilyParser(BG96_AtPacket_t* atPacket, RxData_t* data);

void BG96_txAtCmd(char* packet)
{
    UART2_writeBytes(packet);

    dumpInterComm("[BG96 <-] ");
    dumpInterComm(packet);
}

/* Execution Command AT+<cmd> */
void BG96_sendAtPacket(BG96_AtPacket_t* atPacket)
{
    static char atCmdBody[BUFFER_SIZE];
    memset(atCmdBody, '\0', sizeof(atCmdBody));

    strcpy(atCmdBody, "AT");
    strcat(atCmdBody, atPacket->atCmd->cmd);
    
    switch(atPacket->atCmdType)
    {
        case TEST_COMMAND:
            // TODO: TEST_COMMAND
            break;
        case READ_COMMAND:
            strcat(atCmdBody, "?\r\n");
            break;
        case WRITE_COMMAND:
            strcat(atCmdBody, "=");
            strcat(atCmdBody, atPacket->atCmd->arg);
            strcat(atCmdBody, "\r\n");
            break;
        case EXECUTION_COMMAND:
            strcat(atCmdBody, "\r\n");
            break;
        default:
            break;
    }

    BG96_txAtCmd(atCmdBody);
}

/* Create FreeRTOS tasks */ // TODO: should control task sizes uxTaskGetStackHighWaterMark()
void createTaskRx(void)
{
    xTaskCreate(
                taskRx,                 /* Task function */
                "taskRx",               /* Name of task */
                2048,                   /* Stack size of task */
                NULL,                   /* Parameter of the task */
                tskIDLE_PRIORITY+3,     /* Priority of the task */
                &taskRxHandle           /* Handle of created task */
                );
}

void createTaskTx(void)
{
    xTaskCreate(
                taskTx,                 /* Task function */
                "taskTx",               /* Name of task */
                2048,                   /* Stack size of task */
                NULL,                   /* Parameter of the task */
                tskIDLE_PRIORITY+2,     /* Priority of the task */
                &taskTxHandle           /* Handle of created task */
                );
}

void createTaskPowerUpModem(gpio_num_t pwrKeyPin)
{
    xTaskCreate(
                taskPowerUpModem,       /* Task function */
                "taskPowerUpModem",     /* Name of task */
                1024,                   /* Stack size of task */
                (void*) pwrKeyPin,      /* Parameter of the task */
                tskIDLE_PRIORITY,       /* Priority of the task */
                &taskPowerUpModemHandle /* Handle of created task */
                );
}

void createTaskFeedTxQueue(void)
{
    xTaskCreate(
                taskFeedTxQueue,        /* Task function */
                "taskFeedTxQueue",      /* Name of task */
                2048,                   /* Stack size of task */
                NULL,                   /* Parameter of the task */
                tskIDLE_PRIORITY+1,     /* Priority of the task */
                &taskFeedTxQueueHandle  /* Handle of created task */
                );
}

/* Create FreeRTOS queues */
void createRxQueue(void)
{
    rxDataQueue = xQueueCreate(1, sizeof(RxData_t));
}

void createAtPacketsTxQueue(void)
{
    // Create queue that holds varibles of type BG96_AtPacket_t
    atPacketsTxQueue = xQueueCreate(10, sizeof(BG96_AtPacket_t));
}

/* FreeRTOS tasks */
static void taskPowerUpModem(void *pvParameters)
{
    static uint8_t i = 0;
    static uint16_t timeToWaitForPowerUp = 8000;
    static gpio_num_t pwrKeypin;
    static RxData_t rxData;

    pwrKeypin = (gpio_num_t)pvParameters;
    powerUpModem(pwrKeypin);

    printInfo("\r\nModem power-up: [ STARTED ]\r\n");
    while(1)
    {
        if (xQueueReceive(rxDataQueue, &rxData, MS_TO_TICKS(250)) == pdTRUE)
        {
            if (strstr(rxData.b, "APP RDY") != NULL)
            {
                printInfo("Modem power-up: [ SUCCESS ]\r\n");
                createTaskFeedTxQueue();
                break;
            }
        }

        if (i++ > (timeToWaitForPowerUp/250))
        {
            printInfo("Modem power-up: [ FAIL ]\r\n");
            break;
        }
    }
    vTaskDelete(NULL);
}

static void taskFeedTxQueue(void* pvParameters)
{
    static TaskState_t taskState = TASK_RUN;

    createTaskTx();
    TASK_DELAY_MS(4000); // give BG96 time to connect to GSM network

    while(1)
    {
        switch(taskState)
        {
            case TASK_RUN:
                queueAtPacket(&AT_setCommandEchoMode, EXECUTION_COMMAND);
                queueAtPacket(&AT_enterPIN, READ_COMMAND);
                queueAtPacket(&AT_signalQualityReport, EXECUTION_COMMAND);
                queueAtPacket(&AT_networkRegistrationStatus, READ_COMMAND);
                queueAtPacket(&AT_attachmentOrDetachmentOfPS, READ_COMMAND);
                
                initCommonConnParams();
                BG96_configureSslParams();
                BG96_configureMqttParams();
                BG96_configureTcpIpParams();
                BG96_connectToMqttServer();
                
                // swPowerDownModem();
                taskState = TASK_IDLE;
                break;
            case TASK_IDLE:
                // TODO: problably not necessary, deleteTask?
                break;
            default:
                break;
        }
        TASK_DELAY_MS(100);
    }
}

static void taskTx(void *pvParameters)
{
    while(1)
    {
        if (xQueueReceive(atPacketsTxQueue, &deqdAtPacket, MS_TO_TICKS(20)) == pdTRUE)
        {
            for(uint8_t i = 0; i < RESEND_ATTEMPTS; i++)
            {
                BG96_sendAtPacket(&deqdAtPacket);
            
                if (responseParser() == EXIT_SUCCESS)
                {
                    xTaskNotifyGive(taskFeedTxQueueHandle);
                    break;
                }
            }
        }
    }
}

static uint8_t responseParser(void)
{
    static RxData_t rxData;

    if (xQueueReceive(rxDataQueue, &rxData, MS_TO_TICKS(deqdAtPacket.atCmd->maxRespTime_ms)) == pdTRUE)
    {
        if (strstr(rxData.b, deqdAtPacket.atCmd->cmd) != NULL) // make sure the response belongs to the cmd that was sent
        {
            if (strstr(rxData.b, deqdAtPacket.atCmd->confirmation) != NULL)
            {
                dumpInterComm("[BG96 ->] ");
                dumpInterComm(rxData.b);
                // printInfo("Response: [ OK ]\r\n");
                BG96_atCmdFamilyParser(&deqdAtPacket, &rxData);
                return EXIT_SUCCESS;
            }
            else if(strstr(rxData.b, deqdAtPacket.atCmd->error) != NULL)
            {
                printInfo("Response: [ ERROR ]\r\n");
                return EXIT_FAILURE;
            }
            else if (xQueueReceive(rxDataQueue, &rxData, MS_TO_TICKS(deqdAtPacket.atCmd->maxRespTime_ms)) == pdTRUE)
            {
                if (strstr(rxData.b, deqdAtPacket.atCmd->confirmation) != NULL)
                {
                    printInfo("Response: [ LATE OK ]\r\n");
                    BG96_atCmdFamilyParser(&deqdAtPacket, &rxData);
                    return EXIT_SUCCESS;
                }
                else if(strstr(rxData.b, deqdAtPacket.atCmd->error) != NULL)
                {
                    printInfo("Response: [ LATE ERROR ]\r\n");
                    return EXIT_FAILURE;
                }
            }
            else if (strstr(rxData.b, ">") != NULL)
            {
                BG96_atCmdFamilyParser(&deqdAtPacket, &rxData);
                return EXIT_SUCCESS;
            }
        }
    }
    else
    {
        printInfo("Response: [ MISSING ]\r\n");
        return EXIT_FAILURE;
    }

    return EXIT_FAILURE;
}

static void BG96_atCmdFamilyParser(BG96_AtPacket_t* atPacket, RxData_t* data)
{
    switch(atPacket->atCmd->family)
    {
        case TCPIP_RELATED_AT_COMMANDS:
            BG96_tcpipOkResponseParser(atPacket, data->b);
            break;
        case MQTT_RELATED_AT_COMMANDS:
            BG96_mqttOkResponseParser(atPacket, data->b);
            break;
        default:
            break;
    }
}

static void taskRx(void* pvParameters)
{
    static int readLen;
    static RxData_t rxData;

    while(1)
    {
        readLen = uart_read_bytes(UART_NUM_2, rxData.b, BUFFER_SIZE, MS_TO_TICKS(5));
        if (readLen > 0)
        {
            queueRxData(rxData);
            memset(rxData.b, '\0', sizeof(rxData.b));
        }
    }
}

static void powerUpModem(gpio_num_t pwrKeypin) 
{
    gpio_set_level(pwrKeypin, 1);
    TASK_DELAY_MS(BG96_HOLD_POWER_UP_PIN_MS);
    gpio_set_level(pwrKeypin, 0);
}

static void swPowerDownModem(void) 
{
    queueAtPacket(&AT_powerDown, EXECUTION_COMMAND);
}

static void queueRxData(RxData_t rxData)
{
    xQueueSend(rxDataQueue, rxData.b, 10);
}

void queueAtPacket(AtCmd_t* cmd, AtCmdType_t cmdType)
{
    BG96_AtPacket_t xAtPacket;
    static uint32_t notifValue;
    static uint8_t firstAtPacket = 1;

    xAtPacket.atCmd = cmd;
    xAtPacket.atCmdType = cmdType;

    notifValue = ulTaskNotifyTake(pdFALSE, MS_TO_TICKS(2*(cmd->maxRespTime_ms)));
    if (notifValue > 0)
    {
        xQueueSend(atPacketsTxQueue, &xAtPacket, 0);
    }
    else if (firstAtPacket == 1)
    {
        firstAtPacket = 0;
        xQueueSend(atPacketsTxQueue, &xAtPacket, 0);
    }
    else
    {
        printInfo("ulTaskNotifyTake: [ EXPIRED ]");
    }
}

void prepareArg(char** paramsArr, uint8_t numOfParams, char* arg)
{
    memset(arg, '\0', ARG_LEN);
    for (uint8_t i = 0; i < numOfParams; i++)
    {
        if (paramsArr[i] != NULL)
        {
            strcat(arg, paramsArr[i]);
            strcat(arg, ",");
        }
    }
    arg[strlen(arg)-1] = '\0';
}

static void initCommonConnParams(void)
{
    memset(contextIdStr, '\0', sizeof(contextIdStr));
    sprintf(contextIdStr, "%d", contextID);
    
    memset(sslCtxIdStr, '\0', sizeof(sslCtxIdStr));
    sprintf(sslCtxIdStr, "%d", SSL_ctxID);

    memset(clientIdxStr, '\0', sizeof(clientIdxStr));
    sprintf(clientIdxStr, "%d", client_idx);
}


void dumpInterComm(char* msg)
{
#ifdef DUMP_INTER_COMM
    UART0_writeBytes(msg);
#endif
}

void printInfo(char* msg)
{
#ifdef PRINT_INFO
    UART0_writeBytes(msg);
#endif
}


// void createTaskTest(void)
// {
//     xTaskCreate(
//                 taskTest,        /* Task function */
//                 "taskTest",      /* Name of task */
//                 1024,                   /* Stack size of task */
//                 NULL,                   /* Parameter of the task */
//                 tskIDLE_PRIORITY+1,     /* Priority of the task */
//                 NULL                    /* Handle of created task */
//                 );
// }

// static void taskTest(void *pvParameters)
// {
//     RxData_t response;
//     while(1)
//     {
//         if (xQueueReceive(rxDataQueue, &response, MS_TO_TICKS(20)) == pdTRUE)
//         {
//             UART0_writeBytes("TEST:");
//             UART0_writeBytes(response.b);
//         }
//     }   
// }