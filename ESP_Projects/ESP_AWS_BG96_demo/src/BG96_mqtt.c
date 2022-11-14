#include "BG96_mqtt.h"

static uint8_t mqttConnOpened = 0;
static uint8_t mqttConnConnected = 0;

void BG96_configureMqttParams(void)
{
    static char* paramsArr[MAX_PARAMS];
    static uint8_t idx = 0;
    char tempStr[8];

    static MqttSslMode_t SSL_enable = USE_SECURE_SSL_TCP_FOR_MQTT;
    static MqttProtocolVersion_t vsn = MQTT_V_3_1_1;
    
    idx = 0;
    paramsArr[idx++] = "\"ssl\"";
    paramsArr[idx++] = clientIdxStr;
    sprintf(tempStr, "%d", SSL_enable);
    paramsArr[idx++] = tempStr;
    paramsArr[idx++] = sslCtxIdStr;
    prepareArg(paramsArr, idx, AT_configureOptionalParametersOfMQTT.arg);
    queueAtPacket(&AT_configureOptionalParametersOfMQTT, WRITE_COMMAND); 

    idx = 0;
    paramsArr[idx++] = "\"version\"";
    paramsArr[idx++] = clientIdxStr;
    sprintf(tempStr, "%d", vsn);
    paramsArr[idx++] = tempStr;
    prepareArg(paramsArr, idx, AT_configureOptionalParametersOfMQTT.arg);
    queueAtPacket(&AT_configureOptionalParametersOfMQTT, WRITE_COMMAND); 
}


void BG96_connectToMqttServer(void)
{
    static char* paramsArr[MAX_PARAMS];
    static uint8_t idx = 0;
    static uint16_t port = 8883;
    char tempStr[8];
    char tempStr2[8];
    char tempStr3[8];
    static uint8_t msgID = 1; // TODO: add enums
    static uint8_t qos = 1;
    static uint8_t retain = 0;
    static char* mqttTopic = "\"BG96_demoThing/sensors\"";

    idx = 0;
    paramsArr[idx++] = clientIdxStr;
    paramsArr[idx++] = DEVICE_DATA_ENDPOINT_STR;
    sprintf(tempStr, "%d", port);
    paramsArr[idx++] = tempStr;
    prepareArg(paramsArr, idx, AT_openNetworkConnectionForMQTTClient.arg);
    queueAtPacket(&AT_openNetworkConnectionForMQTTClient, WRITE_COMMAND); 

    while(1) // TODO: replace by timer
    {
        static uint8_t i = 0;
        TASK_DELAY_MS(250);
        if (mqttConnOpened == 1) // takes approx 3.8s
        {
            idx = 0;
            paramsArr[idx++] = clientIdxStr;
            paramsArr[idx++] = MQTT_BG96DEMO_CLIENT_ID_STR;
            prepareArg(paramsArr, idx, AT_connectClientToMQTTServer.arg);
            queueAtPacket(&AT_connectClientToMQTTServer, WRITE_COMMAND);
            break;
        }

        if (i++ > 30)
        {
            break;
        }
    }

    while(1) // TODO: replace by timer
    {
        static uint8_t i = 0;
        TASK_DELAY_MS(20);
        if (mqttConnConnected == 1) // takes approx 200ms
        {
            idx = 0;
            paramsArr[idx++] = clientIdxStr;
            sprintf(tempStr, "%d", msgID);
            paramsArr[idx++] = tempStr;
            sprintf(tempStr2, "%d", qos);
            paramsArr[idx++] = tempStr2;
            sprintf(tempStr3, "%d", retain);
            paramsArr[idx++] = tempStr3;
            paramsArr[idx++] = mqttTopic;
            prepareArg(paramsArr, idx, AT_publishMessages.arg);
            queueAtPacket(&AT_publishMessages, WRITE_COMMAND);
            break;
        }

        if (i++ > 30)
        {
            break;
        }
    }
}

void BG96_mqttOkResponseParser(BG96_AtPacket_t* packet, char* data)
{
    BG96_AtPacket_t tempPacket;
    char tempData[BUFFER_SIZE];
    RxData_t rxData;

    memcpy(&tempPacket, packet, sizeof(BG96_AtPacket_t));
    memcpy(tempData, data, BUFFER_SIZE);

    switch (tempPacket.atCmd->id)
    {
        case CONFIGURE_OPTIONAL_PARAMETERS_OF_MQTT:

            break;
        case OPEN_A_NETWORK_CONNECTION_FOR_MQTT_CLIENT:
            if (tempPacket.atCmdType == WRITE_COMMAND)
            {
                if (xQueueReceive(rxDataQueue, &rxData, MS_TO_TICKS(tempPacket.atCmd->maxRespTime_ms)) == pdTRUE)
                {
                    if(strstr(rxData.b, "+QMTOPEN: 0,0") != NULL) // TODO: implement parser!
                    {
                        printInfo("MQTT connection: [ OPENED ]\r\n");
                        mqttConnOpened = 1;
                    }
                    else
                    {
                        printInfo("MQTT connection: [ DENIED ]\r\n");
                    }
                }
            }
            break;
        case CLOSE_A_NETWORK_FOR_MQTT_CLIENT:

            break;
        case CONNECT_A_CLIENT_TO_MQTT_SERVER:
        if (tempPacket.atCmdType == WRITE_COMMAND)
            {
                if (xQueueReceive(rxDataQueue, &rxData, MS_TO_TICKS(tempPacket.atCmd->maxRespTime_ms)) == pdTRUE)
                {
                    if(strstr(rxData.b, "+QMTCONN: 0,0,0") != NULL) // TODO: implement parser!
                    {
                        printInfo("MQTT connection: [ SUCCESS ]\r\n");
                        mqttConnConnected = 1;
                    }
                    else
                    {
                        printInfo("MQTT connection: [ FAILED ]\r\n");
                    }
                }
            }
            break;
        case DISCONNECT_A_CLIENT_FROM_MQTT_SERVER:

            break;
        case SUBSCRIBE_TO_TOPICS:

            break;
        case PUBLISH_MESSAGES:
            if (tempPacket.atCmdType == WRITE_COMMAND)
            {
                if(strstr(tempData, ">") != NULL)
                {
                    // TODO: implement inputPayload() function
                    char subStr[2];
                    subStr[0] = (char)26;
                    subStr[1] = '\0';
                    UART2_writeBytes("{ \"sensorName\" : \"tempSensor\", \"data\" : 111}");
                    UART2_writeBytes(subStr);
                    UART0_writeBytes("{ \"sensorName\" : \"tempSensor\", \"data\" : 111}");
                }
            }
            break;
        default:
            break;
    }
}