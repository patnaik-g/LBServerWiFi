#ifndef ASYNC_DEBUG_H
#define ASYNC_DEBUG_H

#include <Arduino.h>

/* Configuration */
#define DEBUG_QUEUE_DEPTH 32
#define DEBUG_MSG_LEN 256 // Increased to handle long LocoNet packets

namespace Debug {
    // Explicitly declare the queue as extern for global linkage
    extern QueueHandle_t debugQueue;

    void begin();
    void log(const char* fmt, ...);
}

#define LOG_DEBUG(fmt, ...) Debug::log(fmt, ##__VA_ARGS__)

#endif
