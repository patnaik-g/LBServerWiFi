#include "AsyncDebug.h"
#include <TelnetStream.h>

namespace Debug {
    
    // Structure for queue items
    struct LogMessage {
        char text[DEBUG_MSG_LEN];
    };

    static QueueHandle_t debugQueue = NULL;

    /**
     * @brief Background Task: Consumes messages and prints them.
     * Runs at minimal priority to avoid interfering with LocoNet/WiFi.
     */
    void debugTask(void *pvParameters) {
        LogMessage msg;
        
        for (;;) {
            // Block indefinitely until a message arrives
            if (xQueueReceive(debugQueue, &msg, portMAX_DELAY) == pdPASS) {
                
                // 1. Print to Serial if available
                if (Serial) {
                    Serial.print(msg.text);
                    // Add implicit newline if your previous LOG_DEBUG expected it, 
                    // or rely on the format string containing \n.
                }

                // 2. Print to Telnet if a client is connected
                TelnetStream.print(msg.text);
            }
        }
    }

    void begin() {
        // Create the queue
        debugQueue = xQueueCreate(DEBUG_QUEUE_DEPTH, sizeof(LogMessage));

        // Create the task
        // Priority 0 (IDLE) | Stack 2048 | No Affinity (Any Core)
        xTaskCreate(debugTask, "DebugTask", 2048, NULL, tskIDLE_PRIORITY, NULL);
    }

    void log(const char* fmt, ...) {
        if (!debugQueue) return;

        LogMessage msg;
        va_list args;
        va_start(args, fmt);
        
        // Safe formatting into the fixed-size buffer
        vsnprintf(msg.text, sizeof(msg.text), fmt, args);
        
        va_end(args);

        // Push to queue. Do not block if full (drop message to preserve system timing)
        xQueueSend(debugQueue, &msg, 0);
    }
}
