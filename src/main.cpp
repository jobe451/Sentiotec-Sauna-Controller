#include <Arduino.h>

#define RXD2 16
#define TXD2 17
#define SerialTxControl 19

const unsigned long requestSaunaStatusEveryMs = 8000;
unsigned long lastRequestSaunaStatusMs = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(57600, SERIAL_8N1, RXD2, TXD2);
  pinMode(SerialTxControl, OUTPUT);
  digitalWrite(SerialTxControl, LOW);
} 


void preTransmission()
{
  digitalWrite(SerialTxControl, HIGH);
  Serial.println("Transmit On"); 
  delay(80);
}

void postTransmission()
{
  delay(80);
  digitalWrite(SerialTxControl, LOW);
  Serial.println("Transmit Off");
}

void getSaunaStatus() {
  preTransmission();
  Serial.print("get info\n"); 
  Serial2.print("get info\n");
  Serial2.flush();
  postTransmission();
}

void loop() {
  if (millis() - lastRequestSaunaStatusMs > requestSaunaStatusEveryMs) {
      getSaunaStatus();
      lastRequestSaunaStatusMs = millis();
  }
  Serial.println(lastRequestSaunaStatusMs); 

  Serial.println(Serial2.readString());
  delay(200);
}