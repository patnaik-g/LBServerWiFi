#ifndef ASYNC_DEBUG_H
#define ASYNC_DEBUG_H

#include "Config.h"
#include <Arduino.h>

namespace Debug {
#if defined(ENABLE_SERIAL_LOGGING) || defined(ENABLE_TELNET_LOGGING)
    extern QueueHandle_t debugQueue;
    void begin();
    void log(const char* fmt, ...);
    #define LOG_DEBUG(fmt, ...) Debug::log(fmt, ##__VA_ARGS__)
#else
    inline void begin() {}
    #define LOG_DEBUG(fmt, ...) ((void)0)
#endif
}
#endif
