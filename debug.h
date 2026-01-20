#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include <TelnetStream.h>

#define LOG_DEBUG(fmt, ...) { \
    if (Serial) { \
        Serial.printf(fmt, ##__VA_ARGS__); \
        Serial.flush(); \
    } \
    TelnetStream.printf(fmt, ##__VA_ARGS__); \
    TelnetStream.flush(); \
    yield(); \
}

#endif