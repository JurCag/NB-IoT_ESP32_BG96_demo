
#define TINY_GSM_MODEM_BG96

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial2

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Define how you're planning to connect to the internet
// These defines are only for this example; they are not needed in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

#define TINY_GSM_MODEM_HAS_GPRS
#define TINY_GSM_MODEM_HAS_WIFI

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "lpwa.vodafone.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "Go-Fi";
const char wifiPass[] = "Narcisova6";

// MQTT details
const char *broker = "broker.hivemq.com";

const char *topicLed       = "GSMClientTest/led";
const char *topicInit      = "GSMClientTest/init";
const char *topicLedStatus = "GSMClientTest/ledStatus";

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <Ticker.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#define POUZIVA_SA_WIFI
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#define POUZIVA_SA_GPRS
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif


TinyGsmClient client(modem);
PubSubClient  mqtt(client);

Ticker tick;

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      17
#define PIN_RX      16
#define PWR_PIN     23

#define LED_PIN     19

int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;

void mqttCallback(char *topic, byte *payload, unsigned int len)
{

    SerialMon.print("Message arrived [");
    SerialMon.print(topic);
    SerialMon.print("]: ");
    SerialMon.write(payload, len);
    SerialMon.println();

    // Only proceed if incoming message's topic matches
    // Tu by mala byt ale nejaka kontrola ze prijata sprava sa =="toggle" nie?
    if (String(topic) == topicLed) {
        ledStatus = !ledStatus;
        digitalWrite(LED_PIN, ledStatus);
        SerialMon.print("ledStatus:");
        SerialMon.println(ledStatus);
        mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");

    }
}

boolean mqttConnect()
{
    static uint8_t inc;
    static char str[22] = "GSMClientTest START  ";

    SerialMon.print("[1] Connecting to");
    SerialMon.print(broker);

    // Connect to MQTT Broker
    boolean status = mqtt.connect("GSMClientTest");

    // Or, if you want to authenticate MQTT:
    // boolean status = mqtt.connect("GsmClientName", "mqtt_user", "mqtt_pass");
    // boolean status = mqtt.connect("GSMClientTest", "LilyGO", "b5qyh4365u");// nefunguje

    if (status == false) {
        SerialMon.println(" fail[1]");
        return false;
    }
    SerialMon.println(" success[1]");
    
    inc++;
    str[21] = (char) (inc + '0');
    mqtt.publish(topicInit, str);
    mqtt.subscribe(topicLed);
    return mqtt.connected();
}


void setup()
{
    // Set console baud rate
    Serial.begin(115200, SERIAL_8N1, 3, 1);
    delay(10);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    for (int i = 0; i <4; i++)
    {
        ledStatus = !ledStatus;
        digitalWrite(LED_PIN, ledStatus);
        ledStatus = !ledStatus;
        delay(500);
        digitalWrite(LED_PIN, ledStatus);
        delay(500);
    }

    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(750);
    digitalWrite(PWR_PIN, LOW);

    delay(5000);

    Serial.println("\nWait...");

    delay(1000);

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    Serial.println("Initializing modem...");
    if (!modem.restart()) {
        Serial.println("Failed to restart modem, attempting to continue without restarting");
    }



    // String name = modem.getModemName();
    // DBG("Modem Name:", name);

    // String modemInfo = modem.getModemInfo();
    // DBG("Modem Info:", modemInfo);

    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && modem.getSimStatus() != 3) {
        modem.simUnlock(GSM_PIN);
    }

    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
        SerialMon.println(" fail");
        delay(10000);
        return;
    }
    SerialMon.println(" success");

    if (modem.isNetworkConnected()) {
        SerialMon.println("Network connected");
    }

    // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("[2] Connecting to "));
    SerialMon.print(apn);
    SerialMon.print("\r\n");
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println(" fail[2]");
        delay(10000);
        return;
    }
    SerialMon.println(" success[2]");

    if (modem.isGprsConnected()) {
        SerialMon.println("GPRS connected[2]");
    }

    // MQTT Broker setup
    mqtt.setServer(broker, 1883);
    mqtt.setCallback(mqttCallback);

}

void loop()
{
    // SerialMon.println("NEXT ITERATION");

    // Make sure we're still registered on the network
    if (!modem.isNetworkConnected()) 
    {
        SerialMon.println("Network disconnected");
        if (!modem.waitForNetwork(180000L, true)) {
            SerialMon.println(" fail");
            delay(10000);
            return;
        }
        if (modem.isNetworkConnected()) {
            SerialMon.println("Network re-connected");
        }

        // and make sure GPRS/EPS is still connected
        if (!modem.isGprsConnected()) {
            SerialMon.println("[3] GPRS disconnected!");
            SerialMon.print(F("[3] Connecting to "));
            SerialMon.print(apn);
            if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
                SerialMon.println(" fail[3]");
                delay(10000);
                return;
            }
            if (modem.isGprsConnected()) {
                SerialMon.println("[3] GPRS reconnected");
            }
        }
    }

    if (!mqtt.connected()) {
        SerialMon.println("=== MQTT NOT CONNECTED ===");
        // Reconnect every 10 seconds
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 10000L) {
            lastReconnectAttempt = t;
            if (mqttConnect()) {
                lastReconnectAttempt = 0;
            }
        }
        delay(100);
        return;
    }

    mqtt.loop();

}