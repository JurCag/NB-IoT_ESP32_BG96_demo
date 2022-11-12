#define TINY_GSM_DEBUG Serial
#define CAYENNE_PRINT Serial
#define TINY_GSM_MODEM_BG96


#define USE_GSM  //! Uncomment will use SIM7000 for GSM communication

#ifdef USE_GSM
#include <CayenneMQTTGSM.h>
#else
#include <CayenneMQTTESP32.h>
#endif
#include <Arduino.h>
#include <Wire.h>
// #include <Adafruit_BMP085.h>


#define TEMPERATURE_VIRTUAL_CHANNEL         1
#define BAROMETER_VIRTUAL_CHANNEL           2
#define ALTITUDE_VIRTUAL_CHANNEL            3
#define BATTERY_VIRTUAL_CHANNEL             4
#define SOLAR_VIRTUAL_CHANNEL               5
#define LIGHTSENSOR_VIRTUAL_CHANNEL         6

#define PIN_TX      17
#define PIN_RX      16
#define UART_BAUD   115200
#define PWR_PIN     23

#define BAT_ADC     35
#define SOLAR_ADC   36

// Adafruit_BMP085 bmp;
HardwareSerial  gsmSerial(2);

#ifdef USE_GSM
    // GSM connection info.
    char apn[] = "lpwa.vodafone.com"; // Access point name. Leave empty if it is not needed.
    char gprsLogin[] = ""; // GPRS username. Leave empty if it is not needed.
    char gprsPassword[] = ""; // GPRS password. Leave empty if it is not needed.
    char pin[] = ""; // SIM pin number. Leave empty if it is not needed.
#else
    // WiFi network info.
    char ssid[] = "your wifi ssid";
    char wifiPassword[] = "your wifi password";
#endif

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "b43519e0-1743-11ed-baf6-35fab7fd0ac8";
char password[] = "5c347e544a0aeda1a9933e251d14e75f2beb83b5";
char clientID[] = "2affe940-17b4-11ed-baf6-35fab7fd0ac8";

// bool bmpSensorDetected = true;



void setup()
{
    Serial.begin(115200, SERIAL_8N1, 3, 1);
    delay(10);
    gsmSerial.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    pinMode(PWR_PIN, OUTPUT);

    //Launch BG96
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, HIGH);
    delay(750);
    digitalWrite(PWR_PIN, LOW);
    delay(5000);


#ifdef USE_GSM
    Cayenne.begin(username, password, clientID, gsmSerial, apn, gprsLogin, gprsPassword, pin);
#else
    Cayenne.begin(username, password, clientID, ssid, wifiPassword);
#endif
}

unsigned long myTime;

void loop()
{
    Cayenne.loop();     // Data are uploaded every 15 seconds (because pollVirtualChannels(); is called every 15000ms)
    myTime = millis();
}



// Default function for processing actuator commands from the Cayenne Dashboard.
// You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
    CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
    //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
}

CAYENNE_IN(1)
{
    CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
}


// This function is called at intervals to send temperature sensor data to Cayenne.
CAYENNE_OUT(TEMPERATURE_VIRTUAL_CHANNEL)
{

    if (true) {
        float temperature = random(1, 100) / 10.0;

        Serial.print("Temperature = ");
        Serial.print(temperature);
        Serial.println(" *C");

        Cayenne.celsiusWrite(TEMPERATURE_VIRTUAL_CHANNEL, temperature);
    }
}

// This function is called at intervals to send barometer sensor data to Cayenne.
CAYENNE_OUT(BAROMETER_VIRTUAL_CHANNEL)
{
    if (true) {
        float pressure = random(10, 100000) / 10.0;

        Serial.print("Pressure = ");
        Serial.print(pressure );
        Serial.println(" hPa");

        Cayenne.hectoPascalWrite(BAROMETER_VIRTUAL_CHANNEL, pressure);
    }
}


CAYENNE_OUT(ALTITUDE_VIRTUAL_CHANNEL)
{
    if (true) {
        float altitude = random(0, 15000) / 10.0;

        Serial.print("Altitude = ");
        Serial.print(altitude);
        Serial.println(" meters");

        Cayenne.virtualWrite(ALTITUDE_VIRTUAL_CHANNEL, altitude, "meters", UNIT_METER);

    }
}

float readBattery(uint8_t pin)
{
    int vref = 1100;
    // uint16_t volt = analogRead(pin);
    uint16_t volt = random(45000, 52000) / 10.0;
    float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref);
    return battery_voltage;
}


CAYENNE_OUT(BATTERY_VIRTUAL_CHANNEL)
{
    float mv = random(45000, 52000) / 10.0; //readBattery(BAT_ADC);
    Serial.printf("batter : %f\n", mv);
    Cayenne.virtualWrite(BATTERY_VIRTUAL_CHANNEL, mv, TYPE_VOLTAGE, UNIT_MILLIVOLTS);

}


CAYENNE_OUT(SOLAR_VIRTUAL_CHANNEL)
{
    float mv = random(45000, 52000) / 10.0; //readBattery(SOLAR_ADC);
    Serial.printf("solar : %f\n", mv);
    Cayenne.virtualWrite(SOLAR_VIRTUAL_CHANNEL, mv, TYPE_VOLTAGE, UNIT_MILLIVOLTS);

    Serial.println((float) (myTime/ 1000.0), 3);
}