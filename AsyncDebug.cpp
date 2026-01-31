#include "AsyncDebug.h"
#include <TelnetStream.h>

namespace Debug {
#if defined(ENABLE_SERIAL_LOGGING) || defined(ENABLE_TELNET_LOGGING)
    struct LogMessage { char text[DEBUG_MSG_LEN]; };
    QueueHandle_t debugQueue = NULL;

    void debugTask(void *pvParameters) {
        LogMessage msg;
        for (;;) {
            if (xQueueReceive(debugQueue, &msg, portMAX_DELAY) == pdPASS) {
                #ifdef ENABLE_SERIAL_LOGGING
                if (Serial) Serial.print(msg.text);
                #endif
                
                #ifdef ENABLE_TELNET_LOGGING
                TelnetStream.print(msg.text);
                #endif
            }
        }
    }

    void begin() {
        if (debugQueue != NULL) return;
        debugQueue = xQueueCreate(DEBUG_QUEUE_DEPTH, sizeof(LogMessage));
        // Stack: 4096 words (16KB) for stability
        xTaskCreate(debugTask, "DebugTask", 4096, NULL, tskIDLE_PRIORITY, NULL);
    }

    void log(const char* fmt, ...) {
        if (!debugQueue) return;
        LogMessage msg;
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg.text, sizeof(msg.text), fmt, args);
        va_end(args);
        xQueueSend(debugQueue, &msg, 0);
    }
#endif
}
