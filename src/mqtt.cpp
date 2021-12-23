#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "util.h"

namespace MQTT {
    namespace   // anonymous namespace for private stuff
    {
        WiFiClient wifiClient;
        PubSubClient MQTTclient(wifiClient);
        unsigned long mqttCheckLastTimestamp = 0;
        unsigned long mqttCheckMinMillisBetweenChecks = 40000;
        bool mqttIsInitialized = false;
        const char* mqttHostName;
        int mqttLED;
        char* mqtt_prefix;
/*
       char logBuf[160]; // Buffer for all log-messages
         void mqttCallback(const char *topic, const byte *payload, uint32_t length)
        {
            char *receivedString = strndup((char *)payload, length);
            char *mqttTopic = strdup(topic);

            snprintf(logBuf, sizeof(logBuf) / sizeof(logBuf[0]), "MQTT-Nachricht empfangen: [Topic: %s] [Kommando: %s]", mqttTopic, receivedString);
            Serial.println(logBuf);

            free(receivedString);
            free(mqttTopic);
        }
  */
    }

    void init(Preferences &prefsSettings, const char* _mqttHostName, int _mqttLED) {
        mqttHostName = _mqttHostName;
        int mqtt_port = prefsSettings.getInt("MQTT_PORT", 1883);

        String mqtt_serverStr = prefsSettings.getString("MQTT_SERVER", "");
        char* mqtt_server = util::strToChar(mqtt_serverStr);

        String mqtt_prefixStr = prefsSettings.getString("MQTT_PREFIX", "/devices/SaunaController");
        mqtt_prefix = util::strToChar(mqtt_prefixStr);

        Serial.print("MQTT Server is. ");
        Serial.println(mqtt_server);

        Serial.print("MQTT Prefix is. ");
        Serial.println(mqtt_prefix);

        mqttLED = _mqttLED;
        if (mqttLED > 0) {
            pinMode(mqttLED, OUTPUT);
            digitalWrite(mqttLED, LOW);
        }

        if (strcmp(mqtt_server, "") == 0) {
            Serial.println("no mqtt server configured");
            return;
        }

        mqttHostName = _mqttHostName;
        MQTTclient.setKeepAlive(60);
        MQTTclient.setServer(mqtt_server, mqtt_port);

        mqttIsInitialized = true;
    }

    char* removePrefixFromTopic(char *topic) {
        String topicStr = String(topic);
        String topicStrRed = topicStr.substring(strlen(mqtt_prefix));
        return util::strToChar(topicStrRed);
    }

    void subscribe(const char *topic) {
        char fullTopic[160];
        strcpy(fullTopic, mqtt_prefix); 
        strcat(fullTopic, topic);
        Serial.print("subscribe ");
        Serial.println(fullTopic);
        MQTTclient.subscribe(fullTopic, 0);
    }

    void setCallback(std::function<void (char *, uint8_t *, unsigned int)> callback) {
        MQTTclient.setCallback(callback);
    }

    bool publishMqtt(const char *topic, const char *payload, bool retained) {
        String topicStr = String(topic);
        char fullTopic[160];

        if (topicStr.startsWith(mqtt_prefix)) {
            strcpy(fullTopic, topic); 
        }
        else {
            strcpy(fullTopic, mqtt_prefix); 
            strcat(fullTopic, topic);
        }


        if (strcmp(fullTopic, "") != 0)
        {
            if (MQTTclient.connected())
            {
                MQTTclient.publish(fullTopic, payload, retained);
                delay(100);
                return true;
            }
        }
        return false;
    }

    bool publishMqtt(const char *topic, String &payload, bool retained) {
        return publishMqtt(topic, payload.c_str(), retained);
    }

    bool publishMqtt(const char *topic, int32_t payload, bool retained) {
        char buf[11];
        snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%d", payload);
        return publishMqtt(topic, buf, retained);
    }

    bool publishMqtt(const char *topic, unsigned long payload, bool retained) {
        char buf[11];
        snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%lu", payload);
        return publishMqtt(topic, buf, retained);
    }

    bool publishMqtt(const char *topic, uint32_t payload, bool retained) {
        char buf[11];
        snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%u", payload);
        return publishMqtt(topic, buf, retained);
    }

    bool publishMqtt(const char *topic, float payload, bool retained) {
        char buf[11];
        snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%f", payload);
        return publishMqtt(topic, buf, retained);
    }

    bool publishMqtt(const char *topic, double payload, bool retained) {
        char buf[11];
        snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%f", payload);
        return publishMqtt(topic, buf, retained);
    }

    boolean isConnected() {
        return MQTTclient.connected();
    }

    /* Connects/reconnects to MQTT-Broker unless connection is not already available.
        Manages MQTT-subscriptions.
    */
    bool mqttReconnect() {
        if (!mqttIsInitialized) {
            return false;
        }

        boolean isConnected = MQTTclient.connected(); 
        if (isConnected) {
            if (mqttLED > 0) {
                digitalWrite(mqttLED, HIGH);
            }
            return isConnected;
        }
        if (mqttLED > 0) {
            digitalWrite(mqttLED, LOW);
        }

        if (mqttCheckLastTimestamp > 0 && millis() - mqttCheckLastTimestamp < mqttCheckMinMillisBetweenChecks) {
            return isConnected;
        }

        uint8_t maxRetries = 10;
        uint8_t mqttFailCount = 0; // Number of times mqtt-reconnect is allowed to fail. If >= mqttFailCount to further reconnects take place

        while (!MQTTclient.connected() && mqttFailCount < maxRetries) {
            // Try to connect to MQTT-server
            if (MQTTclient.connect(mqttHostName)) {
                if (mqttLED > 0) {
                    digitalWrite(mqttLED, HIGH);
                }
                Serial.println("MQTT connected");
            }
            else {
                mqttFailCount++;
                delay(500);
            }
        }

        mqttCheckLastTimestamp = millis();
        return MQTTclient.connected();
    }

    void loop() {
        if (!mqttIsInitialized) {
            return;
        }
        MQTTclient.loop();
    }
}

