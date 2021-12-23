#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>


namespace MQTT {
    void init(Preferences &prefsSettings, const char* _mqttHostName, int mqttLED);
    void setCallback(std::function<void (char *, uint8_t *, unsigned int)> callback);
    char* removePrefixFromTopic(char *topic);
    void subscribe(const char *topic);
    bool publishMqtt(const char *topic, const char *payload, bool retained);
    bool publishMqtt(const char *topic, String &payload, bool retained);
    bool publishMqtt(const char *topic, const char *payload, bool retained);
    bool publishMqtt(const char *topic, int32_t payload, bool retained);
    bool publishMqtt(const char *topic, unsigned long payload, bool retained);
    bool publishMqtt(const char *topic, uint32_t payload, bool retained);
    bool publishMqtt(const char *topic, float payload, bool retained);
    bool publishMqtt(const char *topic, double payload, bool retained);
    bool mqttReconnect();
    boolean isConnected();
    void loop();
}




