// Test loggovania sprav na debugovanie
// Link -> 
// https://community.platformio.org/t/espressif32-esp-idf-how-to-enable-debug-mode-via-platformio-ini/2280/2
// vid platformio.ini

// Otvor PlatformIO cli a napis pio device monitor -p COM7
// Zacne pocuvat na porte COM7 (pripadne pridaj -b 115200 ale netreba)
// mozno otvorit az ked sa nahra kod ale, inak bude nepristupny
// lenze nestihnes uvidiet spravu tak otvor ho a potom stlac RST button na doske
#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h> 
#include <freertos/task.h>
#include <driver/gpio.h>

void taskDelayTestPin18(void *pvParameters);
void taskDelayTestPin19(void *pvParameters);

static const char *TAG = "logging_demo";

void app_main(void) 
{
    printf("Hello World");
    int log_level = 1;

    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    esp_log_level_t current_log_level = esp_log_level_get(TAG);

    printf("\n\nCurrent level: %d\n\n", current_log_level);

    ESP_LOGE(TAG, "This is Error Logging with level %d\n", log_level++);
    ESP_LOGW(TAG, "This is Warning Logging with level %d\n", log_level++);
    ESP_LOGI(TAG, "This is Error Info with level %d\n", log_level++);

    // Neviem preco tieto 2 uz nelogguje, aj ked level som nastavil az na ESP_LOG_VERBOSE
    // pozri mozno tu: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html
    ESP_LOGD(TAG, "This is Error Debug with level %d\n", log_level++);
    ESP_LOGV(TAG, "This is Error Verbose with level %d\n", log_level++);

    gpio_reset_pin(19);
    gpio_set_direction(19, GPIO_MODE_OUTPUT);
    gpio_set_level(19, 0);
    gpio_reset_pin(18);
    gpio_set_direction(18, GPIO_MODE_OUTPUT);
    gpio_set_level(18, 0);

    xTaskCreate(
                taskDelayTestPin18, /* Task function. */
                "taskUart0",        /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY+2,   /* priority of the task */
                NULL                /* Task handle to keep track of created task */
                );

    xTaskCreate(
                taskDelayTestPin19, /* Task function. */
                "taskUart0",        /* name of task. */
                1024,               /* Stack size of task */
                NULL,               /* parameter of the task */
                tskIDLE_PRIORITY+2,   /* priority of the task */
                NULL                /* Task handle to keep track of created task */
                );
}

// A) Ak je nastavene CONFIG_FREERTOS_HZ = 1000 (v sdkconfig.h)
//      -> 1 "tick" znamena cakanie 1ms
// B) Ak je nastavene CONFIG_FREERTOS_HZ = 100 (v sdkconfig.h) - defaultne
//      -> 1 "tick" znamena cakanie 10ms
//      na 100Hz nemozme po nom chcet aby cakal kratsie ako 10ms

// with "/portTICK_PERIOD_MS" should be in ms
void taskDelayTestPin18(void *pvParameters)
{
    while(1)
    {
        gpio_set_level(18, 0);
        vTaskDelay(1/portTICK_PERIOD_MS); // A) 1ms, B) 12.5us (divna nedefinovana hodnota)
        gpio_set_level(18, 1);
        vTaskDelay(1/portTICK_PERIOD_MS); // A) 1ms, B) 12.5us (divna nedefinovana hodnota)
    }
}

// without "/portTICK_PERIOD_MS" should be in "freeRTOS ticks"
void taskDelayTestPin19(void *pvParameters)
{
    while(1)
    {
        gpio_set_level(19, 0); // A) 1ms, B) 10ms
        vTaskDelay(1);
        gpio_set_level(19, 1); // A) 1ms, B) 10ms
        vTaskDelay(1);
    }
}