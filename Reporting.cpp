#include "Reporting.h"
#include <TelnetStream.h>

void reportingTask(void *pvParameters) {
    ReportMsg msg;
    
    for (;;) {
        // This task sleeps until a message arrives (zero CPU usage)
        if (xQueueReceive(reportQueue, &msg, portMAX_DELAY) == pdPASS) {
            if (Serial) {
                Serial.println(msg.text);
            }
            TelnetStream.println(msg.text);
        }
    }
}