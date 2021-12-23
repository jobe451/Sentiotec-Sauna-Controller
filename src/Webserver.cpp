#include <Arduino.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include "Webserver.h"
#include "Wifi.h"
#include "basicWebsite.h"


namespace Webserver {

    namespace   // anonymous namespace for private stuff
    {
        AsyncWebServer _wServer(80);
        Preferences _prefsSettings;
        Wifi *_wifi;
        int _enableGpio;
        bool _toogleAccessPoint = false;

        void buttonInterruptHandler() {
            if (digitalRead(_enableGpio) == LOW) { //only act on keydown
                Serial.println("Interrupt LOW");
                return;
            }
            Serial.println("Interrupt HIGH");
            _toogleAccessPoint = true;
        }

        String webserverTemplateProcessor(const String &templ) {
            if (templ == "SSID") {
                return _prefsSettings.getString("SSID", "");
            }
            else if (templ == "MQTT_SERVER") {
                return _prefsSettings.getString("MQTT_SERVER", "");
            }
            else if (templ == "MQTT_PORT") {
                int port = _prefsSettings.getInt("MQTT_PORT", 1883);
                char buffer [50];
                sprintf (buffer, "%d", port);
                return String(buffer);
            }
            else if (templ == "MQTT_PREFIX") {
                return _prefsSettings.getString("MQTT_PREFIX", "/devices/SaunaController");
            }
            else {
                return "N/A";
            }

            return String();
        }
    }
    


    void init(Preferences &prefsSettings, Wifi &wifi, int enableGpio)
    {
        _prefsSettings = prefsSettings;
        _wifi = &wifi;
        _enableGpio = enableGpio;

        if (enableGpio > 0) {
            pinMode(enableGpio, INPUT);
            attachInterrupt(enableGpio, buttonInterruptHandler, CHANGE);
        }

        _wServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            Serial.println("Config page requested");
            request->send_P(200, "text/html", basicWebsite, webserverTemplateProcessor);
        });

        _wServer.on("/", HTTP_POST, [&prefsSettings, &wifi] (AsyncWebServerRequest *request) {
            Serial.println("to store page posted");
            if (request->hasParam("ssid", true) && request->hasParam("pwd", true)) {
                Serial.print("Settings update received, SSID: ");
                Serial.println(request->getParam("ssid", true)->value());
                prefsSettings.putString("SSID", request->getParam("ssid", true)->value());
                prefsSettings.putString("Password", request->getParam("pwd", true)->value());
                prefsSettings.putString("MQTT_SERVER", request->getParam("mqttServer", true)->value());
                prefsSettings.putInt("MQTT_PORT", atoi(request->getParam("mqttPort", true)->value().c_str()));
                prefsSettings.putString("MQTT_PREFIX", request->getParam("mqttPrefix", true)->value());
            }
            request->send_P(200, "text/html", confirmationWebsite, webserverTemplateProcessor);
            Serial.flush();
            delay(1000);
            ESP.restart();
        });

        if(_wifi->isAccessPointModeInTransition()) {
            startServer();
        }
    }

    void observeWebserver() {
        if (!_toogleAccessPoint) {
            return;
        }

        _toogleAccessPoint = false;
        Serial.println("toogle it");

        if (_wifi->isAccessPointModeInTransition()) {
            Serial.println("aborting switching accesspoint mode, mode is in transition");
            return;
        }
        if(_wifi->isInAccessPointMode()) {
            stopServer();
            _wifi->accessPointStop();
        }
        else {
            Serial.println("switch to AP mode");
            _wifi->accessPointStart();
            delay(600);
            startServer();
        }
    }

    void startServer() {
        _wServer.begin();
        Serial.println("webserver started");
    }

    void stopServer() {
        _wServer.end();
        Serial.println("webserver stopped, going for reboot");
    }
}

