#include "NetworkInterface.h"
#include <LocoNetStreamESP32.h> // Required for consolidated hardware logic
#include "debug.h"

/* Hardware Configuration - Consistently defined here for hardware baseline */
#define LOCONET_PIN_RX 22
#define LOCONET_PIN_TX 23

// THE DEFINITIONS - These exist exactly once in the project after merger
LocoNetBus bus;
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1, LOCONET_PIN_RX, LOCONET_PIN_TX, false, true, &bus);

/*
 * UTILITY: getPacketLen
 */
uint8_t getPacketLen(const lnMsg *p) {
    uint8_t opc = p->data[0];
    switch (opc & 0x60) {
        case 0x00: return 2;
        case 0x20: return 4;
        case 0x40: return 6;
        case 0x60: return p->data[1];
        default: return 0;
    }
}

QueueHandle_t lnToNetQueue;
QueueHandle_t netToLnQueue;

void setup() {
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) { delay(10); } 
 
    // Increased queue depth slightly to handle bursty LocoNet traffic
    lnToNetQueue = xQueueCreate(32, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(32, sizeof(lnMsg));

    lnStream.start(); // Restored from original hardware baseline
    parser.onPacket(CALLBACK_FOR_ALL_OPCODES, [](const lnMsg *p) {
        xQueueSend(lnToNetQueue, p, 0); 
    });
    xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);
    LOG_DEBUG("LBServer v1.7.5 Initialized\n");
}

void loop() {
    lnStream.process();
    
    // Core 1 timing critical - Static allocation avoids stack churning
    static lnMsg tx;
    
    if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
        lnStream.send(&tx);
    }
}
