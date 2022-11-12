#include "BG96.h"

/* Local variables */
static BG96_AtPacket_t deqdAtPacket;
static QueueHandle_t rxDataQueue = NULL;
static QueueHandle_t atPacketsTxQueue = NULL;
static TaskHandle_t taskRxHandle = NULL;
static TaskHandle_t taskTxHandle = NULL;
static TaskHandle_t taskPowerUpModemHandle = NULL;
static TaskHandle_t taskFeedTxQueueHandle = NULL;


/* Local FreeRTOS tasks */
static void taskFeedTxQueue(void* pvParameters);
static void taskRx(void* pvParameters);
static void taskTx(void *pvParameters);
static void taskPowerUpModem(void *pvParameters);
// static void taskTest(void *pvParameters);

/* Local functions */
static void powerUpModem(gpio_num_t pwrKeypin) ;
static void queueRxData(RxData_t rxData);
static void queueAtPacket(AtCmd_t* cmd, AtCmdType_t type);
static void configureSslParams(void);
static void prepareArg(const char* paramsArr[MAX_PARAMS], char* arg);


void BG96_txPacket(char* packet)
{
    UART0_writeBytes(packet);
    UART2_writeBytes(packet);
}

/* Execution Command AT+<cmd> */
void BG96_sendCommand(AtCmd_t* atCmd, AtCmdType_t cmdType)
{
    static char atCmdBody[BUFFER_SIZE];
    memset(atCmdBody, '\0', sizeof(atCmdBody));

    strcpy(atCmdBody, "AT");
    strcat(atCmdBody, atCmd->cmd);
    
    switch(cmdType)
    {
        case TEST_COMMAND:
            // TODO: TEST_COMMAND
            break;
        case READ_COMMAND:
            strcat(atCmdBody, "?\r\n");
            break;
        case WRITE_COMMAND:
            strcat(atCmdBody, "=");
            strcat(atCmdBody, atCmd->arg);
            strcat(atCmdBody, "\r\n");
            break;
        case EXECUTION_COMMAND:
            strcat(atCmdBody, "\r\n");
            break;
        default:
            break;
    }

    BG96_txPacket(atCmdBody);

}

/* Create FreeRTOS tasks */
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
                1024,                   /* Stack size of task */
                NULL,                   /* Parameter of the task */
                tskIDLE_PRIORITY+1,     /* Priority of the task */
                &taskFeedTxQueueHandle  /* Handle of created task */
                );
}

/* Create FreeRTOS queues */
void createRxQueue(void)
{
    rxDataQueue = xQueueCreate(4, sizeof(RxData_t));
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

    UART0_writeBytes("\r\nModem power-up: [ STARTED ]\r\n");
    while(1)
    {
        if (xQueueReceive(rxDataQueue, &rxData, MS_TO_TICKS(250)) == pdTRUE)
        {
            if (strstr(rxData.b, "APP RDY") != NULL)
            {
                UART0_writeBytes("Modem power-up: [ SUCCESS ]\r\n");
                createTaskFeedTxQueue();
                break;
            }
            xTaskNotifyGive(taskRxHandle);
        }

        if (i++ > (timeToWaitForPowerUp/250))
        {
            UART0_writeBytes("Modem power-up: [ FAIL ]\r\n");
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
                configureSslParams();
                taskState = TASK_IDLE;
                break;
            case TASK_IDLE:
                // TODO: nejakou direct message ho znova dat do TASK_RUN? alebo proste suspend/delete ak uz nic nebude robit
                break;
            default:
                break;
        }
        TASK_DELAY_MS(100);
    }
}

static void taskTx(void *pvParameters)
{
    static char expectedResponse[32];
    static char* response;
    static RxData_t rxData;

    while(1)
    {
        if (xQueueReceive(atPacketsTxQueue, &deqdAtPacket, MS_TO_TICKS(20)) == pdTRUE)
        {                        
            BG96_sendCommand(deqdAtPacket.atCmd, deqdAtPacket.atCmdType);
            
            memcpy(expectedResponse, deqdAtPacket.atCmd->cmd, strlen(deqdAtPacket.atCmd->cmd));

            if (xQueueReceive(rxDataQueue, &rxData, MS_TO_TICKS(deqdAtPacket.atCmd->maxRespTime_ms)) == pdTRUE)
            {
                if (deqdAtPacket.atCmdType == WRITE_COMMAND)
                {
                    if (strstr(rxData.b, expectedResponse) != NULL)
                    {
                        xTaskNotifyGive(taskFeedTxQueueHandle);
                    }
                }

                
                response = strstr(rxData.b, deqdAtPacket.atCmd->confirmation);
                if (response != NULL)
                {
                    UART0_writeBytes(rxData.b);
                    UART0_writeBytes(response);
                    // TODO: responseParser()
                    // break;
                }
                else
                {
                    // Search for error response "ERROR"
                    response = strstr(rxData.b, deqdAtPacket.atCmd->error);
                    // break;
                }
            }
            // TODO: else odoslat znovu 3x?, ak stale nedojde odpoved tak nejaky error stav
        }
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
        }
    }
}

static void powerUpModem(gpio_num_t pwrKeypin) 
{
    gpio_set_level(pwrKeypin, 1);
    TASK_DELAY_MS(BG96_HOLD_POWER_UP_PIN_MS);
    gpio_set_level(pwrKeypin, 0);
}

static void queueRxData(RxData_t rxData)
{
    xQueueSend(rxDataQueue, rxData.b, 10);
}

static void queueAtPacket(AtCmd_t* cmd, AtCmdType_t cmdType)
{
    static uint8_t firstWriteCmd = 1;
    BG96_AtPacket_t xAtPacket;
    uint32_t notifValue;

    xAtPacket.atCmd = cmd;
    xAtPacket.atCmdType = cmdType;
    
    if (cmdType != WRITE_COMMAND)
    {
        xQueueSend(atPacketsTxQueue, &xAtPacket, MS_TO_TICKS(400));
    }
    else
    {
        notifValue = ulTaskNotifyTake(pdFALSE, MS_TO_TICKS(2*(cmd->maxRespTime_ms)));
        if (notifValue > 0)
        {
            xQueueSend(atPacketsTxQueue, &xAtPacket, MS_TO_TICKS(400));
        }
        else if (firstWriteCmd == 1)
        {
            firstWriteCmd = 0;
            xQueueSend(atPacketsTxQueue, &xAtPacket, MS_TO_TICKS(400));
        }
        else
        {
            UART0_writeBytes("ulTaskNotifyTake expired");
        }
    }
}


static void configureSslParams(void)
{
    char ctxIdStr[16];
    char tempStr[16];
    static char* paramsArr[MAX_PARAMS];

    static SslContextID_t SSL_ctxID = SSL_CTX_ID_0; 
    static SslVersion_t SSL_version = ALL;
    static SslSecLevel_t seclevel = MANAGE_SERVER_AND_CLIENT_AUTHENTICATION;
    static SslIgnoreLocalTime_t ignore_ltime = IGNORE_VALIDITY_CHECK;
    static char cacertpath[] = "\"cacert.pem\"";
    static char client_cert_path[] = "\"client.pem\"";
    static char client_key_path[] = "\"user_key.pem\"";
    static char supportAllCiphersuites[] = "0xFFFF";
    
    memset(ctxIdStr, '\0', sizeof(ctxIdStr));
    sprintf(ctxIdStr, "%d", SSL_ctxID);

    paramsArr[0] = "\"cacert\"";
    paramsArr[1] = ctxIdStr;
    paramsArr[2] = cacertpath;
    prepareArg(paramsArr, AT_configureParametersOfAnSSLContext.arg);
    queueAtPacket(&AT_configureParametersOfAnSSLContext, WRITE_COMMAND); 

    paramsArr[0] = "\"clientcert\"";
    paramsArr[1] = ctxIdStr;
    paramsArr[2] = client_cert_path;
    prepareArg(paramsArr, AT_configureParametersOfAnSSLContext.arg);
    queueAtPacket(&AT_configureParametersOfAnSSLContext, WRITE_COMMAND); 

    paramsArr[0] = "\"clientkey\"";
    paramsArr[1] = ctxIdStr;
    paramsArr[2] = client_key_path;
    prepareArg(paramsArr, AT_configureParametersOfAnSSLContext.arg);
    queueAtPacket(&AT_configureParametersOfAnSSLContext, WRITE_COMMAND); 

    paramsArr[0] = "\"ciphersuite\"";
    paramsArr[1] = ctxIdStr;
    paramsArr[2] = supportAllCiphersuites;
    prepareArg(paramsArr, AT_configureParametersOfAnSSLContext.arg);
    queueAtPacket(&AT_configureParametersOfAnSSLContext, WRITE_COMMAND); 

    paramsArr[0] = "\"sslversion\"";
    paramsArr[1] = ctxIdStr;
    sprintf(tempStr, "%d", SSL_version);
    paramsArr[2] = tempStr;
    prepareArg(paramsArr, AT_configureParametersOfAnSSLContext.arg);
    queueAtPacket(&AT_configureParametersOfAnSSLContext, WRITE_COMMAND);

    paramsArr[0] = "\"seclevel\"";
    paramsArr[1] = ctxIdStr;
    sprintf(tempStr, "%d", seclevel);
    paramsArr[2] = tempStr;
    prepareArg(paramsArr, AT_configureParametersOfAnSSLContext.arg);
    queueAtPacket(&AT_configureParametersOfAnSSLContext, WRITE_COMMAND);

    paramsArr[0] = "\"ignorelocaltime\"";
    paramsArr[1] = ctxIdStr;
    sprintf(tempStr, "%d", ignore_ltime);
    paramsArr[2] = tempStr;
    prepareArg(paramsArr, AT_configureParametersOfAnSSLContext.arg);
    queueAtPacket(&AT_configureParametersOfAnSSLContext, WRITE_COMMAND);
}

static void prepareArg(const char* paramsArr[MAX_PARAMS], char* arg)
{
    memset(arg, '\0', ARG_LEN);
    for (uint8_t i = 0; i < MAX_PARAMS; i++)
    {
        if (paramsArr[i] != NULL)
        {
            strcat(arg, paramsArr[i]);
            strcat(arg, ",");
        }
    }
    arg[strlen(arg)-1] = '\0';
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