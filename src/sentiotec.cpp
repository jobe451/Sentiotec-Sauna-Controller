#include <Arduino.h>
#include <map>
#include "RS485.h"
#include "mqtt.h"
#include "util.h"

namespace Sentiotec {

    namespace   // anonymous namespace for private stuff
    {
        boolean _isConnected = false;
        const int _commandCountDownStart = 15;
        int _sentioAvailableCnt = 0;
        const int _sentioInAvailableAfter = _commandCountDownStart * 3;
        int _sentioLED;
        const int publishEqualContent = 100;
        int _currentBlinkState = LOW;
        int _getCommandsIterationCnt = 0;

        const char* SENTIOTEC_SAUNA_STAT = "get sauna";
        const char* SENTIOTEC_SAUNA_VAL = "get sauna val";
        const char* SENTIOTEC_STEAM_STAT = "get steam";
        const char* SENTIOTEC_STEAM_VAL = "get steam val";
        const char* SENTIOTEC_LIGHT_STAT = "get light";
        const char* SENTIOTEC_LIGHT_VAL = "get light val";
        const char* SENTIOTEC_FAN_STAT = "get fan";
        const char* SENTIOTEC_FAN_VAL = "get fan val";
        const char* SENTIOTEC_I_SWITCH_STAT = "get i-switch";
        const char* SENTIOTEC_I_SWITCH_VAL = "get i-switch val";
        const char* SENTIOTEC_TIMER_STAT = "get timer";
        const char* SENTIOTEC_TIMER_VAL = "get timer val";
        const char* SENTIOTEC_TEMP_HEATER_VAL = "get temp-heater val";
        const char* SENTIOTEC_TEMP_BENCH_VAL = "get temp-bench val";
        const char* SENTIOTEC_HUMIDITY_VAL = "get humidity val";
        const char* SENTIOTEC_FOIL_VAL = "get foil-temp val";
        const char* SENTIOTEC_HEATTIMER_STAT = "get heattimer";
        const char* SENTIOTEC_HEATTIMER_VAL = "get heattimer val";
        const char* SENTIOTEC_USER_STAT = "get user-prog";
        const char* SENTIOTEC_USER_VAL = "get user-prog val";
        const char* SENTIOTEC_DRYPROG = "get dryprog";
        const char* SENTIOTEC_CONFIG = "get config";
        const char* SENTIOTEC_INFO = "get info";
        const char* SENTIOTEC_STATUS = "get status";

        const char* PATH_INFO = "/values/info";
        const char* PATH_STATUS = "/values/status";
        const char* PATH_SAUNA = "/settings/sauna";
        const char* PATH_SAUNA_TEMPC = "/settings/sauna_tempC";
        const char* PATH_LIGHT = "/settings/light";
        const char* PATH_LIGHT_VAL = "/settings/light_val";
        const char* PATH_FAN = "/settings/fan";
        const char* PATH_FAN_VAL = "/settings/fan_val";
        const char* PATH_STEAM = "/settings/steam";
        const char* PATH_STEAM_HUMIDITY = "/settings/steam_humidity";
        const char* PATH_TIMER = "/settings/timer";
        const char* PATH_TIMER_MIN = "/settings/timer_min";
        const char* PATH_HEATTIMER = "/settings/heattimer";
        const char* PATH_HEATTIMER_MIN = "/settings/heattimer_min";
        const char* PATH_I_SWITCH = "/settings/i-switch";
        const char* PATH_I_SWITCH_VAL = "/settings/i-switch_val";
        const char* PATH_HEATER_TEMPC = "/values/heater_tempC";
        const char* PATH_BENCH_TEMPC = "/values/bench_tempC";
        const char* PATH_FOIL_TEMPC = "/values/foil_tempC";
        const char* PATH_HUMIDITY_PERC = "/values/humidity_perc";
        const char* PATH_DRYPROG = "/values/dryprog";
        const char* PATH_CONFIG = "/values/config";
        const char* PATH_USER_PROG = "/settings/user-prog";
        const char* PATH_USER_PROG_VAL = "/settings/user-prog_val";

        std::map<char*, char*> valueCash;


        const int _commandSize = 29;
        const char* _commands[] = {
            SENTIOTEC_SAUNA_STAT, 
            SENTIOTEC_SAUNA_VAL, 
            SENTIOTEC_STEAM_STAT, 
            SENTIOTEC_STEAM_VAL, 
            SENTIOTEC_LIGHT_STAT, 
            SENTIOTEC_LIGHT_VAL, 
            SENTIOTEC_FAN_STAT, 
            SENTIOTEC_SAUNA_STAT, 
            SENTIOTEC_SAUNA_VAL, 
            SENTIOTEC_FAN_VAL, 
            SENTIOTEC_I_SWITCH_STAT, 
            SENTIOTEC_I_SWITCH_VAL, 
            SENTIOTEC_TIMER_STAT, 
            SENTIOTEC_TIMER_VAL, 
            SENTIOTEC_SAUNA_STAT, 
            SENTIOTEC_SAUNA_VAL, 
            SENTIOTEC_TEMP_HEATER_VAL, 
            SENTIOTEC_TEMP_BENCH_VAL, 
            SENTIOTEC_HUMIDITY_VAL, 
            SENTIOTEC_FOIL_VAL, 
            SENTIOTEC_HEATTIMER_STAT, 
            SENTIOTEC_HEATTIMER_VAL, 
            SENTIOTEC_SAUNA_STAT, 
            SENTIOTEC_SAUNA_VAL, 
            SENTIOTEC_USER_STAT, 
            SENTIOTEC_USER_VAL, 
            SENTIOTEC_DRYPROG, 
            SENTIOTEC_CONFIG, 
            SENTIOTEC_INFO, 
            SENTIOTEC_STATUS
        };

        RS485 _rs485;
        int _commandId = 0;
        int _commandCountDown = _commandCountDownStart;
        char logBuf[160]; // Buffer for all log-messages
        char* _lastCommandRequested = (char*)SENTIOTEC_SAUNA_STAT;

        boolean isCashedValueSameAndUpdateCash(char* token, char* info) {
            if (valueCash.count(token)) {
                char* previousValue = valueCash[token];         
                if (strcmp(previousValue, info) == 0) {
                    return true; // no change, no need to Update
                }
            }
            valueCash[token] = info;
            return false;
        }

        void publish(char* token, char* info) {
            int randNummber = rand() % publishEqualContent; // publishs the value every x time anyway, to recover form potential asynchronous data caused be connection interruption 
            if (randNummber == 0 || isCashedValueSameAndUpdateCash(token, info)) {
                return;
            }
            MQTT::publishMqtt(token, info, false);
        }

        void publish(char* token, String &info) {
            char* infoChar = util::strToChar(info);
            publish(token, infoChar);
        }

        boolean validateOnOff(char* data) {
            if (strcmp(data, "on") == 0) {
                return true;
            }
            if (strcmp(data, "off") == 0) {
                return true;
            }
            Serial.print("invalid string received: ");
            Serial.println(data);
            return false;
        }

        char* validateNumeric(char* data) {
            if (strlen(data) > 4) {
                Serial.println("to large string");
                return (char*) "";
            }

            String dataStr(data);
            dataStr.trim();
            const long intVal = dataStr.toInt();
            if (intVal != NULL && intVal >= 0 && intVal <= 9999) {
                char* outChar = util::strToChar(dataStr);
                Serial.println(outChar);
                return outChar;
            }
            Serial.println("out of range");
            return (char*) "";
        }

        void sendOnOffCommand(char* receivedString, const char* commandPrefix) {
            if (validateOnOff(receivedString)) {
                char buf[50];
                strcpy(buf, commandPrefix);
                strcat(buf, receivedString);
                Serial.println(buf);
                _rs485.writeCommand(buf);
            }
        }

        void sendNummericCommand(char* receivedString, const char* commandPrefix) {
            char* validatedInput = validateNumeric(receivedString);
            if (strlen(validatedInput) > 0) {
                char buf[50];
                strcpy(buf, commandPrefix);
                strcat(buf, receivedString);
                _rs485.writeCommand(buf);
            }
        }

        boolean ignorePubs() {
            return _getCommandsIterationCnt <= 2;
        }

        void mqttCallback(const char *topic, const byte *payload, uint32_t length) {
            char *receivedString = strndup((char*)payload, length);
            char *mqttTopicFull = strdup(topic);

            char* mqttTopic = MQTT::removePrefixFromTopic(mqttTopicFull);
            snprintf(logBuf, sizeof(logBuf) / sizeof(logBuf[0]), "MQTT-Nachricht: Topic: %s: Kommando: %s", mqttTopic, receivedString);
            Serial.println(logBuf);

            if (ignorePubs()) {
                Serial.println("Commands are ignored for 20sec after startup");
                return;
            }

            if (isCashedValueSameAndUpdateCash((char*) mqttTopic, receivedString)) {
                Serial.println("IS SAME!!!!!!!!!!!!!!!!!!");
                return;
            }

            if (strcmp(mqttTopic, PATH_SAUNA) == 0) {
                sendOnOffCommand(receivedString, "set sauna ");
            }

            else if (strcmp(mqttTopic, PATH_SAUNA_TEMPC) == 0) {
                sendNummericCommand(receivedString, "set sauna val ");
            }

            else if (strcmp(mqttTopic, PATH_LIGHT) == 0) {
                sendOnOffCommand(receivedString, "set light ");
            }

            else if (strcmp(mqttTopic, PATH_LIGHT_VAL) == 0) {
                sendNummericCommand(receivedString, "set light val ");
            }

            else if (strcmp(mqttTopic, PATH_FAN) == 0) {
                sendOnOffCommand(receivedString, "set fan ");
            }

            else if (strcmp(mqttTopic, PATH_FAN_VAL) == 0) {
                sendNummericCommand(receivedString, "set fan val ");
            }

            else if (strcmp(mqttTopic, PATH_STEAM) == 0) {
                sendOnOffCommand(receivedString, "set steam ");
            }

            else if (strcmp(mqttTopic, PATH_STEAM_HUMIDITY) == 0) {
                sendNummericCommand(receivedString, "set steam val ");
            }

            else if (strcmp(mqttTopic, PATH_TIMER) == 0) {
                sendOnOffCommand(receivedString, "set timer ");
            }

            else if (strcmp(mqttTopic, PATH_TIMER_MIN) == 0) {
                sendNummericCommand(receivedString, "set timer val ");
            }

            else if (strcmp(mqttTopic, PATH_HEATTIMER_MIN) == 0) {
                sendNummericCommand(receivedString, "set heattimer val ");
            }

            else if (strcmp(mqttTopic, PATH_I_SWITCH) == 0) {
                sendOnOffCommand(receivedString, "set i-switch ");
            }

            else if (strcmp(mqttTopic, PATH_I_SWITCH_VAL) == 0) {
                sendNummericCommand(receivedString, "set i-switch val ");
            }

            else if (strcmp(mqttTopic, PATH_USER_PROG) == 0) {
                sendOnOffCommand(receivedString, "set user-prog ");
            }

            else if (strcmp(mqttTopic, PATH_USER_PROG_VAL) == 0) {
                sendNummericCommand(receivedString, "set user-prog val ");
            }

            else if (strcmp(mqttTopic, PATH_DRYPROG) == 0) {
                sendOnOffCommand(receivedString, "set dryprog ");
            }

            free(receivedString);
            free(mqttTopic);
        }
    }

    void parseMessage(String &response) {
        if (response.startsWith("INFO")) {
            String val = response.substring(6, response.length()-3);
            publish((char*) PATH_INFO, val);
        }
        else if (response.startsWith("STATUS")) {
            String val = response.substring(6, response.length()-3);
            publish((char*) PATH_STATUS, val);
        }
        else if (response.startsWith("SAUNA on")) {
            publish((char*) PATH_SAUNA, (char*) "on");
        }
        else if (response.startsWith("SAUNA off")) {
            publish((char*) PATH_SAUNA, (char*) "off");
        }
        else if (response.startsWith("SAUNA")) {
            String val = response.substring(6, response.length()-5);
            publish((char*) PATH_SAUNA_TEMPC, val);
        }
        else if (response.startsWith("LIGHT on")) {
            publish((char*) PATH_LIGHT, (char*) "on");
        }
        else if (response.startsWith("LIGHT off")) {
            publish((char*) PATH_LIGHT, (char*) "off");
        }
        else if (response.startsWith("LIGHT")) {
            String val = response.substring(6);
            publish((char*) PATH_LIGHT_VAL, val);
        }
        else if (response.startsWith("FAN on")) {
            publish((char*) PATH_FAN, (char*) "on");
        }
        else if (response.startsWith("FAN off")) {
            publish((char*) PATH_FAN, (char*) "off");
        }
        else if (response.startsWith("FAN")) {
            String val = response.substring(4);
            publish((char*) PATH_FAN_VAL, val);
        }
        else if (response.startsWith("STEAM on")) {
            publish((char*) PATH_STEAM, (char*) "on");
        }
        else if (response.startsWith("STEAM off")) {
            publish((char*) PATH_STEAM, (char*) "off");
        }
        else if (response.startsWith("STEAM")) {
            String val = response.substring(6, response.length()-5);
            publish((char*) PATH_STEAM_HUMIDITY, val);
        }
        else if (response.startsWith("TIMER on")) {
            publish((char*) PATH_TIMER, (char*) "on");
        }
        else if (response.startsWith("TIMER off")) {
            publish((char*) PATH_TIMER, (char*) "off");
        }
        else if (response.startsWith("TIMER")) {
            String val = response.substring(6, response.length()-7);
            publish((char*) PATH_TIMER_MIN, val);
        }
        else if (response.startsWith("USER-PROG on")) {
            publish((char*) PATH_USER_PROG, (char*) "on");
        }
        else if (response.startsWith("USER-PROG off")) {
            publish((char*) PATH_USER_PROG, (char*) "off");
        }
        else if (response.startsWith("USER-PROG")) {
            String val = response.substring(6);
            publish((char*) PATH_USER_PROG_VAL, val);
        }
        else if (response.startsWith("HEATTIMER on")) {
            publish((char*) PATH_HEATTIMER, (char*) "on");
        }
        else if (response.startsWith("HEATTIMER off")) {
            publish((char*) PATH_HEATTIMER, (char*) "off");
        }
        else if (response.startsWith("HEATTIMER")) {
            String val = response.substring(10, response.length()-7);
            publish((char*) PATH_HEATTIMER_MIN, val);
        }
        else if (response.startsWith("I-SWITCH on")) {
            publish((char*) PATH_I_SWITCH, (char*) "on");
        }
        else if (response.startsWith("I-SWITCH off")) {
            publish((char*) PATH_I_SWITCH, (char*) "off");
        }
        else if (response.startsWith("I-SWITCH")) {
            String val = response.substring(6);
            publish((char*) PATH_I_SWITCH_VAL, val);
        }
        else if (response.startsWith("TEMP-HEATER")) {
            String val = response.substring(12, response.length()-5);
            publish((char*) PATH_HEATER_TEMPC, val);
        }
        else if (response.startsWith("TEMP-BENCH")) {
            String val = response.substring(11, response.length()-5);
            publish((char*) PATH_BENCH_TEMPC, val);
        }
        else if (response.startsWith("FOIL-BENCH")) {
            String val = response.substring(6);
            publish((char*) PATH_FOIL_TEMPC, val);
        }
        else if (response.startsWith("HUMIDITY")) {
            String val = response.substring(6);
            publish((char*) PATH_HUMIDITY_PERC, val);
        }
        else if (_lastCommandRequested == SENTIOTEC_DRYPROG) {
            publish((char*) PATH_DRYPROG, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_INFO) {
            publish((char*) PATH_INFO, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_STATUS) {
            publish((char*) PATH_STATUS, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_CONFIG) {
            publish((char*)  PATH_CONFIG, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_SAUNA_STAT) {
            publish((char*) PATH_SAUNA, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_SAUNA_VAL) {
            publish((char*) PATH_SAUNA_TEMPC, (char*) "-1");
        }
        else if (_lastCommandRequested == SENTIOTEC_LIGHT_STAT) {
            publish((char*) PATH_LIGHT, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_LIGHT_VAL) {
            publish((char*) PATH_LIGHT_VAL, (char*) "-1");
        }
        else if (_lastCommandRequested == SENTIOTEC_FAN_STAT) {
            publish((char*) PATH_FAN, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_FAN_VAL) {
            publish((char*) PATH_FAN_VAL, (char*) "-1");
        }
        else if (_lastCommandRequested == SENTIOTEC_STEAM_STAT) {
            publish((char*) PATH_STEAM, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_STEAM_VAL) {
            publish((char*) PATH_STEAM_HUMIDITY, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_TIMER_STAT) {
            publish((char*) PATH_TIMER, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_TIMER_VAL) {
            publish((char*) PATH_TIMER_MIN, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_USER_STAT) {
            publish((char*) PATH_USER_PROG, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_USER_VAL) {
            publish((char*) PATH_USER_PROG_VAL, (char*) "-1");
        }
        else if (_lastCommandRequested == SENTIOTEC_HEATTIMER_STAT) {
            publish((char*) PATH_HEATTIMER, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_HEATTIMER_VAL) {
            publish((char*) PATH_HEATTIMER_MIN, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_I_SWITCH_STAT) {
            publish((char*) PATH_I_SWITCH, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_I_SWITCH_VAL) {
            publish((char*) PATH_I_SWITCH_VAL, (char*) "-1");
        }
        else if (_lastCommandRequested == SENTIOTEC_TEMP_HEATER_VAL) {
            publish((char*) PATH_HEATER_TEMPC, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_TEMP_BENCH_VAL) {
            publish((char*) PATH_BENCH_TEMPC, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_FOIL_VAL) {
            publish((char*) PATH_FOIL_TEMPC, response);
        }
        else if (_lastCommandRequested == SENTIOTEC_HUMIDITY_VAL) {
            publish((char*) PATH_HUMIDITY_PERC, response);
        }
    }

    void setLedHigh() {
        if (!ignorePubs()) {
            digitalWrite(_sentioLED, HIGH);
            return;            
        }

        if(_currentBlinkState == HIGH) {
            digitalWrite(_sentioLED, LOW);
            _currentBlinkState = LOW;
        }
        else {
            digitalWrite(_sentioLED, HIGH);
            _currentBlinkState = HIGH;
        }
    }

    void loop() {
        if (!MQTT::isConnected()) {
            if (_sentioLED > 0) {
                digitalWrite(_sentioLED, LOW);
            }
            return;
        }
        _sentioAvailableCnt++;

        String response = _rs485.getResponse();
        if (response.length() > 0) {
            _sentioAvailableCnt = 0;
            if (_sentioLED > 0) {
                setLedHigh();
            }
            Serial.println(response);
            parseMessage(response);
        }

        if (_sentioLED > 0 && _sentioAvailableCnt > _sentioInAvailableAfter) {
            digitalWrite(_sentioLED, LOW);
        }

        if (_commandCountDown == 0) {
            _commandCountDown = _commandCountDownStart;
        }
        _commandCountDown--;

        if (_commandCountDown > 0) {
            return; // only go every 3rd time below here, do more reads then writes
        }
        
        Serial.print(_commandId);
        Serial.print(": ");
        Serial.println((char*) _commands[_commandId]);
        _lastCommandRequested = (char*) _commands[_commandId];
        _rs485.writeCommand((char*) _commands[_commandId]);

        _commandId++;

        if (_commandId > _commandSize) {
            _getCommandsIterationCnt++;
            _commandId = 0;
        } 
        
        if (!_isConnected && MQTT::isConnected() && _getCommandsIterationCnt > 1 && _sentioAvailableCnt < _sentioInAvailableAfter) {
            _isConnected = true;
            MQTT::subscribe(PATH_SAUNA);
            MQTT::subscribe(PATH_SAUNA_TEMPC);
            MQTT::subscribe(PATH_LIGHT);
            MQTT::subscribe(PATH_LIGHT_VAL);
            MQTT::subscribe(PATH_FAN);
            MQTT::subscribe(PATH_FAN_VAL);
            MQTT::subscribe(PATH_STEAM);
            MQTT::subscribe(PATH_STEAM_HUMIDITY);
            MQTT::subscribe(PATH_TIMER);
            MQTT::subscribe(PATH_TIMER_MIN);
            MQTT::subscribe(PATH_HEATTIMER_MIN);
            MQTT::subscribe(PATH_I_SWITCH);
            MQTT::subscribe(PATH_I_SWITCH_VAL);
            MQTT::subscribe(PATH_DRYPROG);
            MQTT::subscribe(PATH_USER_PROG);
            MQTT::subscribe(PATH_USER_PROG_VAL);
        }
        MQTT::loop();
    }

    void init(HardwareSerial& hwSerial, int rxPin,  int txPin, int txControlPin, int sentioLED) {
        _rs485.begin(Serial2, rxPin,  txPin, txControlPin);
        _sentioLED = sentioLED;
        if (_sentioLED > 0) {
            pinMode(_sentioLED, OUTPUT);
            digitalWrite(_sentioLED, LOW);
        }
        MQTT::setCallback(mqttCallback);
    }
}
