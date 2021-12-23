#include <Arduino.h>
#include <WiFi.h>

#ifndef INCLUDED_Wifi
#define INCLUDED_Wifi 
#define WIFI_CHECK_MIN_MILLIS_BETWEEN_CHECKS 10000
#define WIFI_LED_BLINK_INTERVALL 500

class Wifi {
    public:
        Wifi(int ledGpio, IPAddress apIp, IPAddress apNetmask, const char *apSSID);
        void init(const char *ssid, const char *password, const char *hostname);
        bool wifiManager(void);
        void accessPointStart();
        void accessPointStop();
        bool isInAccessPointMode();
        bool isAccessPointModeInTransition();

    private:
        int _ledGpio;
        IPAddress _apIp;
        IPAddress _apNetmask;
        const char *_apSSID;
        wl_status_t _current_wl_status;
        const char *_ssid;
        const char *_password;
        
        WiFiClient _wifiClient;
        IPAddress _myIP;
        unsigned long _wifiCheckLastTimestamp = 0;
        unsigned long _last_blink_start = 0;
        bool _accessPointStarted = false;
        bool _accessPointModeInTransition = false;

        void setLed();
};
#endif 
