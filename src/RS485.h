#include <Arduino.h>

#define PRE_TRANSMISSION_MS 10
#define POST_TRANSMISSION_MS 0


class RS485 {
    public:
        RS485();

        int available();
        void writeCommand(const char *command);
        void begin(HardwareSerial& hwSerial, int rxPin,  int txPin, int txControlPin);
        String getResponse();

    private:
        HardwareSerial* _serial;
        int _rxPin;
        int _txPin;
        int _txControlPin;

        void preTransmission();
        void postTransmission();
};
