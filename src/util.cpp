#include <Arduino.h>

namespace util {

    // needed since String.c_str() does fucking random things
    char* strToChar(String &stringObj) {
        char * outChar = new char[stringObj.length()+1];
        strcpy(outChar, stringObj.c_str());
        return outChar;
    }

}