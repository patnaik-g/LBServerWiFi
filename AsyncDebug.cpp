#include "AsyncDebug.h"
#include <TelnetStream.h>

namespace Debug {
    
    struct LogMessage {
        char text[DEBUG_MSG_LEN];
    };

    // Removed 'static' to allow global linkage via extern
    QueueHandle_t debugQueue = NULL;

    void debugTask(void *pvParameters) {
        LogMessage msg;
        for (;;) {
            if (xQueueReceive(debugQueue, &msg, portMAX_DELAY) == pdPASS) {
                if (Serial) {
                    Serial.print(msg.text);
                }
                TelnetStream.print(msg.text);
            }
        }
    }

    void begin() {
        if (debugQueue != NULL) return; // Prevent double initialization
        debugQueue = xQueueCreate(DEBUG_QUEUE_DEPTH, sizeof(LogMessage));
        xTaskCreate(debugTask, "DebugTask", 2048, NULL, tskIDLE_PRIORITY, NULL);
    }

    void log(const char* fmt, ...) {
        if (!debugQueue) return; // This check now works globally

        LogMessage msg;
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg.text, sizeof(msg.text), fmt, args);
        va_end(args);

        xQueueSend(debugQueue, &msg, 0);
    }
}
