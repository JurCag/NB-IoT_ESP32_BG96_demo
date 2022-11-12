#include <Arduino.h>

#define SerialMon Serial
#define SerialAT Serial2

#define LED           (19)
#define PWRKEY        (23)

#define UART0_TX_PIN  (GPIO_NUM_1)
#define UART0_RX_PIN  (GPIO_NUM_3)
#define UART2_TX_PIN  (GPIO_NUM_17)
#define UART2_RX_PIN  (GPIO_NUM_16)

uint8_t ledState = 0;

void setup() {
  // Set console baud rate
  pinMode(UART0_TX_PIN, OUTPUT);
  pinMode(UART2_TX_PIN, OUTPUT);
  
  SerialMon.begin(115200, SERIAL_8N1, UART0_RX_PIN, UART0_TX_PIN);
  SerialAT.begin(115200, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);

  pinMode(LED, OUTPUT);
  pinMode(PWRKEY, OUTPUT);

  for (int i = 0; i <4; i++)
  {
    ledState = !ledState;
    digitalWrite(LED, ledState);
    ledState = !ledState;
    delay(500);
    digitalWrite(LED, ledState);
    delay(500);
  }

}

void loop() {

  SerialMon.println("Powering-up GSM Modem");
  digitalWrite(PWRKEY, 1);
  delay(750);
  digitalWrite(PWRKEY, 0);
  delay(5000);

  // Access AT commands from Serial Monitor
  SerialMon.println(F("***********************************************************"));
  SerialMon.println(F(" You can now send AT commands"));
  SerialMon.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
  SerialMon.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
  SerialMon.println(F("***********************************************************"));

  while(true) {
    if (SerialAT.available()) { // available if some data from Module received
      SerialMon.write(SerialAT.read());
      ledState = !ledState;
    }
    if (SerialMon.available()) { // available when some data from serial monitor were sent
      SerialAT.write(SerialMon.read());
      ledState = !ledState;
    }
    delay(0);
  }
}