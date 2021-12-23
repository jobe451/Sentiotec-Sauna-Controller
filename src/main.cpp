#include <Arduino.h>
#include <Preferences.h>
#include "Wifi.h"
#include "Webserver.h"
#include "mqtt.h"
#include "util.h"
#include "sentiotec.h"

#define RXD2 16
#define TXD2 17
#define SerialTxControl 19

#define wifiLEDGpio 32
#define mqttLEDGpio 25
#define sentioLEDGpio 33

#define wifiAPgpio 34

static const char prefsSettingsNamespace[] PROGMEM = "settings";    // Namespace used for generic settings
static const char host_name[] PROGMEM = "SaunaController";
static const char apSSID[] PROGMEM = "SaunaControllerAp";
//const char *host_name = "SaunaController";

IPAddress apIP(192, 168, 1, 1);                         // Access-point's static IP
IPAddress apNetmask(255, 255, 255, 0);                  // Access-point's netmask


Preferences prefsSettings;
const unsigned long requestSaunaStatusEveryMs = 100;
unsigned long lastRequestSaunaStatusMs = 0;
Wifi wifi(wifiLEDGpio, apIP, apNetmask, (char *) FPSTR(apSSID));


void setup() {
    if (wifiLEDGpio > 0) {
        pinMode(wifiLEDGpio, OUTPUT);
        digitalWrite(wifiLEDGpio, LOW);
    }
    if (mqttLEDGpio > 0) {
        pinMode(mqttLEDGpio, OUTPUT);
        digitalWrite(mqttLEDGpio, LOW);
    }

    Serial.begin(115200);
    while (!Serial);
    //delay(4000);
    prefsSettings.begin((char *) FPSTR(prefsSettingsNamespace));

    String ssidStr = prefsSettings.getString("SSID", "");
    char* ssid = util::strToChar(ssidStr);
    Serial.print("Wifi SSID is. ");
    Serial.println(ssid);

    String passwordStr = prefsSettings.getString("Password", "");
    char* password = util::strToChar(passwordStr);

    wifi.init(ssid, password, (char *) FPSTR(host_name));
    Webserver::init(prefsSettings, wifi, wifiAPgpio);
    MQTT::init(prefsSettings, (char *) FPSTR(host_name), mqttLEDGpio);
    Sentiotec::init(Serial2, RXD2,  TXD2, SerialTxControl, sentioLEDGpio);
}

void loop() {
    delay(40);
    Webserver::observeWebserver();

    if (!wifi.wifiManager()) {
        if (mqttLEDGpio > 0) {
            pinMode(mqttLEDGpio, OUTPUT);
            digitalWrite(mqttLEDGpio, LOW);
        }
        return;
    }

    if (wifi.isInAccessPointMode()) {
        if (mqttLEDGpio > 0) {
            pinMode(mqttLEDGpio, OUTPUT);
            digitalWrite(mqttLEDGpio, LOW);
        }
        return;
    }

    if (!MQTT::mqttReconnect()) {
        return;
    }

    if (millis() - lastRequestSaunaStatusMs > requestSaunaStatusEveryMs) {
        lastRequestSaunaStatusMs = millis();
        Sentiotec::loop();
    }
    MQTT::loop();
}