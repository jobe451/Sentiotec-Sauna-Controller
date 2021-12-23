#include <Arduino.h>
#include "RS485.h"
#include "mqtt.h"

namespace Sentiotec {
    void init(HardwareSerial& hwSerial, int rxPin,  int txPin, int txControlPin, int sentioLED);
    void loop();
}