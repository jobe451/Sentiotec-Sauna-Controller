#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include "Wifi.h"

namespace Webserver {
    void init(Preferences &prefsSettings, Wifi &wifi, int enableGpio);    
    void observeWebserver();
    void startServer();
    void stopServer();
}




