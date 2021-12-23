#include <Arduino.h>

#ifndef INCLUDED_BASIC_WEBSITE
#define INCLUDED_BASIC_WEBSITE 

static const char basicWebsite[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
    <head>
        <title>Sauna Controller Settings</title>
    </head>
    <body>
        <form action="/" method="POST">
            <fieldset>
                <legend>Initiale Sauna Controller Settings</legend>

                <label for="ssid">SSID:</label><br>
                <input type="text" id="ssid" name="ssid" placeholder="SSID" value="%SSID%" required><br><br>

                <label for="pwd">Passwort:</label><br>
                <input type="password" id="pwd" name="pwd" autocomplete="off" required><br><br>

                <label for="mqttServer">MQTT Servername:</label><br>
                <input type="text" id="mqttServer" name="mqttServer" autocomplete="off" value="%MQTT_SERVER%" required><br><br>

                <label for="mqttPort">MQTT Port:</label><br>
                <input type="text" id="mqttPort" name="mqttPort" autocomplete="off" value="%MQTT_PORT%" required><br><br>

                <label for="mqttPrefix">MQTT Prefix:</label><br>
                <input type="text" id="mqttPrefix" name="mqttPrefix" autocomplete="off" value="%MQTT_PREFIX%" required><br><br>

                <input type="submit" value="update and connect to wifi">
            </fieldset>
        </form>
    </body>
</html>
)rawliteral";

static const char confirmationWebsite[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
    <head>
        <title>Sauna Controller Settings</title>
    </head>
    <body>
        <h1>Data updated</h1>
        <p>restarting...</p>
    </body>
</html>
)rawliteral";

#endif 
