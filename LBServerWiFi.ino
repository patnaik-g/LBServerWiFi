#include "LoconetInterface.h"
#include "NetworkInterface.h"
#include "debug.h"

QueueHandle_t lnToNetQueue;
QueueHandle_t netToLnQueue;

void setup() {
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) { delay(10); } 
 
    lnToNetQueue = xQueueCreate(20, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(20, sizeof(lnMsg));

    lnStream.start();
    parser.onPacket(CALLBACK_FOR_ALL_OPCODES, [](const lnMsg *p) {
        xQueueSend(lnToNetQueue, p, 0); 
    });
    
    xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);
    LOG_DEBUG("LBServer v1.5 Initialized\n");
}

void loop() {
    lnStream.process(); // Core 1 timing critical
    lnMsg tx;
    if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
        lnStream.send(&tx);
    }
}