#include "uart.h"
#include <soc/uart_struct.h>
#include <soc/uart_reg.h>
#include <string.h>

#define PWRKEY_PIN 23

uint8_t uart0RxData[BUFFER_SIZE];
uint8_t uart2RxData[BUFFER_SIZE];
int len0;
int len2;

TaskHandle_t taskInitModemHandle = NULL;
uint8_t replyReceivedFlag;

void powerUpModem(void);
void printSerialMonAndAT(char* msg);

void taskUart0(void *pvParameters)
{
    while (1) 
    {
        vTaskDelay(1);
        // Read data from the UART
        len0 = uart_read_bytes(UART_NUM_0, uart0RxData, BUFFER_SIZE, 50 / portTICK_RATE_MS);
        // Write data back to the UART
        if (len0 > 0)
        {
            uart_write_bytes(UART_NUM_2, (const char *)uart0RxData, len0);
        }
    }
}

void taskUart2(void *pvParameters)
{
    while (1) 
    {
        vTaskDelay(1);
        // Read data from the UART
        len2 = uart_read_bytes(UART_NUM_2, uart2RxData, BUFFER_SIZE, 50 / portTICK_RATE_MS);
        // Write data back to the UART
        if (len2 > 0)
        {
            replyReceivedFlag = 1;
            uart_write_bytes(UART_NUM_0, (const char *) uart2RxData, len2);
        }
    }
}

void taskATcmds(void *pvParameters)
{
    // General commands to verify connection
    printSerialMonAndAT("AT+CPIN?\r\n");
    printSerialMonAndAT("AT+CPIN?\r\n");
    
    printSerialMonAndAT("AT+CSQ\r\n");

    printSerialMonAndAT("AT+CREG?\r\n");

    printSerialMonAndAT("AT+CGATT?\r\n");

    // APN configuration
    printSerialMonAndAT("AT+QICSGP=1,1,\"lpwa.vodafone.com\",\"\",\"\"\r\n");
    
    printSerialMonAndAT("AT+QIACT=1\r\n");

    printSerialMonAndAT("AT+QIACT?\r\n");

    printSerialMonAndAT("AT+QIOPEN=1,0,\"TCP\",\"api.thingspeak.com\",80,0,0\r\n");

    printSerialMonAndAT("AT+QISEND=0\r\n");
    char httpGet[] ="GET https://api.thingspeak.com/update?api_key=MNRPT8HBIFOZG2NR&field1=5.55&field2=22.22\r\n";
    printSerialMonAndAT(httpGet);  

    char arrSUB[] = {(char)26,'\r','\n'};  
    uart_write_bytes(UART_NUM_2, arrSUB, 3); // 'SUB' character
    uart_write_bytes(UART_NUM_0, arrSUB, 3); // 'SUB' character

    vTaskDelay(3000/ portTICK_PERIOD_MS);
    printSerialMonAndAT("AT+QISEND=0,0\r\n");

    vTaskSuspend(NULL);
}

void taskCheckModemInit(void *pvParameters)
{
    powerUpModem();
    UART0_writeBytes("\r\nStarting Check task\r\n",23);
    uint8_t i = 0;
    while (1) 
    {
        vTaskDelay(250/portTICK_PERIOD_MS);
        if (replyReceivedFlag == 1)
        {
            if (strncmp((const char *)"\r\nAPP RDY\r\n", (const char *)uart2RxData, 2) == 0)
            {
                UART0_writeBytes("Modem initialization: [COMPLETE]\r\n",34);
                xTaskCreate(
                    taskATcmds,      /* Task function. */
                    "taskATcmds",    /* name of task. */
                    1024,               /* Stack size of task */
                    NULL,               /* parameter of the task */
                    tskIDLE_PRIORITY,   /* priority of the task */
                    NULL /* Task handle to keep track of created task */
                    );
                break;
            }
        }

        i++;
        if (i > 40)
        {
            UART0_writeBytes("Modem initialization: [FAILED]\r\n",32);
            break;
        }
    }

    UART0_writeBytes("Killing task\r\n", 14);
    // And delete itself
    vTaskDelete(NULL);
}

void app_main() {
    float h = 2.4;
    float t = 24.24;

    gpio_reset_pin(PWRKEY_PIN);
    gpio_set_direction(PWRKEY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PWRKEY_PIN, 0);

    /* UART0 communicates with PC */
    UART0_initConfig();
    UART0_initIO();
    UART0_installDriver();

    /* UART2 forwards data to GSM modem */
    UART2_initConfig();
    UART2_initIO();
    UART2_installDriver();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    xTaskCreate(
                taskUart0,          /* Task function. */
                "taskUart0",        /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY+2,   /* priority of the task */
                NULL                /* Task handle to keep track of created task */
                );

    xTaskCreate(
                taskUart2,          /* Task function. */
                "taskUart2",        /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY+2,   /* priority of the task */
                NULL                /* Task handle to keep track of created task */
                );

    xTaskCreate(
                taskCheckModemInit,      /* Task function. */
                "taskCheckModemInit",    /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY,   /* priority of the task */
                NULL /* Task handle to keep track of created task */
                );

}


void printSerialMonAndAT(char* msg) 
{
    size_t len = strlen(msg);
    UART0_writeBytes(msg, len);
    UART2_writeBytes(msg, len);
    vTaskDelay(1500/portTICK_PERIOD_MS);
}

void powerUpModem(void) 
{
    gpio_set_level(PWRKEY_PIN, 1);
    vTaskDelay(750 / portTICK_PERIOD_MS);
    gpio_set_level(PWRKEY_PIN, 0);
}
