#ifndef ASYNC_DEBUG_H
#define ASYNC_DEBUG_H

#include <Arduino.h>

/* Configuration */
#define DEBUG_QUEUE_DEPTH 32
#define DEBUG_MSG_LEN 128

namespace Debug {
    /**
     * @brief Initializes the debug queue and starts the background logging task.
     * Call this once in setup().
     */
    void begin();

    /**
     * @brief Thread-safe logging function.
     * Formats the string and pushes it to the background queue.
     */
    void log(const char* fmt, ...);
}

// Redirect existing macro to the new async engine
#define LOG_DEBUG(fmt, ...) Debug::log(fmt, ##__VA_ARGS__)

#endif
