#include <WiFi.h>
#include "Wifi.h"

Wifi::Wifi(int ledGpio, IPAddress apIp, IPAddress apNetmask, const char *apSSID):
    _ledGpio(ledGpio),
    _apIp(apIp),
    _apNetmask(apNetmask),
    _apSSID(apSSID),
    _current_wl_status(WL_DISCONNECTED),
    _ssid(""),
    _password("")
{
}

void Wifi::init(const char *ssid, const char *password, const char *hostname)
{
    if (_ledGpio > 0) {
        pinMode(_ledGpio, OUTPUT);
        setLed();
    }
    _ssid = ssid,
    _password = password,
    WiFi.setHostname(hostname);
    WiFi.setAutoReconnect(true);
    _last_blink_start = millis();

    Serial.println("SSID is: ");
    Serial.println(_ssid);
    if (strcmp(_ssid, "") == 0) {
        Serial.println("no wifi settings, starting access point");
        accessPointStart();
        return;
    }

    if (wifiManager()) {
        Serial.println("WiFi started");
    }
    else {
        Serial.println("WiFi start FAILED");
    } 
}

bool Wifi::isInAccessPointMode() {
    return _accessPointStarted;
}

bool Wifi::isAccessPointModeInTransition() {
    return _accessPointModeInTransition;
}

bool Wifi::wifiManager(void)
{
    if (_accessPointStarted) {
        setLed();
        return true;
    }

    _current_wl_status = WiFi.status();
    if (_current_wl_status == WL_CONNECTED )
    {
        setLed();
        return true;
    }

    if (_wifiCheckLastTimestamp > 0 && millis() - _wifiCheckLastTimestamp < WIFI_CHECK_MIN_MILLIS_BETWEEN_CHECKS)
    {
        setLed();
        return false;
    }

    setLed();
    Serial.println("begin wifi");
    WiFi.begin(_ssid, _password); // ESP32-workaround (otherwise WiFi-connection sometimes fails)

    uint8_t tryCount = 0;
    while (_current_wl_status != WL_CONNECTED && tryCount <= 10)
    {
        delay(500);
        Serial.println("connecting to Wifi...");
        tryCount++;
        if (tryCount >= 10 && _current_wl_status == WL_CONNECT_FAILED) {
            WiFi.begin(_ssid, _password);        // ESP32-workaround (otherwise WiFi-connection sometimes fails)
        }
        _current_wl_status = WiFi.status();
    }
    
    if (_current_wl_status == WL_CONNECTED)
    {
        Serial.println("Wifi connected");
        _myIP = WiFi.localIP();
        Serial.print("IP: ");
        Serial.println(_myIP);
    }

    _wifiCheckLastTimestamp = millis();
    setLed();
    return (_current_wl_status == WL_CONNECTED);
}

void Wifi::setLed() {
    if (_ledGpio <= 0) {
        return;
    }
    unsigned long now = millis();

    if (_accessPointStarted) {
        if(((now - _last_blink_start) / WIFI_LED_BLINK_INTERVALL) % 2 == 0) {
            digitalWrite(_ledGpio, LOW);
        }
        else {
            digitalWrite(_ledGpio, HIGH);
        }
        return;
    }

    if (_current_wl_status == WL_CONNECTED) {
        digitalWrite(_ledGpio, HIGH);
        return;
    }

    digitalWrite(_ledGpio, LOW);
}

void Wifi::accessPointStart() {
    _accessPointModeInTransition = true;
    _accessPointStarted = true;
    WiFi.disconnect();
    digitalWrite(_ledGpio, LOW);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apSSID);
    delay(150);
    WiFi.softAPConfig(_apIp, _apIp, _apNetmask);
    IPAddress apIP = WiFi.softAPBroadcastIP();
    Serial.print("IP: ");
    Serial.println(apIP);
    _last_blink_start = millis();
    setLed();
    _accessPointModeInTransition = false;
}

void Wifi::accessPointStop() {
    Serial.println("reboot");
    Serial.flush();
    delay(300);
    ESP.restart();
}

