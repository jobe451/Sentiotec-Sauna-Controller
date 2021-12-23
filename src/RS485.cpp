#include "RS485.h"



RS485::RS485()
{
}

void RS485::preTransmission() {
    digitalWrite(_txControlPin, HIGH);
    delay(PRE_TRANSMISSION_MS);
}

void RS485::postTransmission() {
    delay(POST_TRANSMISSION_MS);
    digitalWrite(_txControlPin, LOW);
}

void RS485::begin(HardwareSerial& hwSerial, int rxPin,  int txPin, int txControlPin) {
    _serial = &hwSerial;
    _rxPin = rxPin;
    _txPin = txPin;
    _txControlPin = txControlPin;
    _serial->begin(57600, SERIAL_8N1, _rxPin, _txPin);
    pinMode(_txControlPin, OUTPUT);
    digitalWrite(_txControlPin, LOW);
}

void RS485::writeCommand(const char *command) {
    preTransmission();
    Serial.print("sending command: ");
    Serial.println(command);
    _serial->println(command);
    _serial->flush();
    postTransmission();   
}

String RS485::getResponse() {
    if(_serial->available()) {
        return _serial->readString(); // Until('\n');
    }
    else {
        return "";
    }
}


