/*
 * LBServerWiFi v2.0.0 - Main Orchestrator
 * PROTOCOL: LBServer (LocoNet-over-TCP)
 * DESCRIPTION: Asynchronous ESP32 bridge between WiFi/TCP and physical LocoNet.
 * Separates timing-critical LocoNet processing from network and logging tasks.
 */
 
#include "NetworkInterface.h"
#include <LocoNetStreamESP32.h> // Required for consolidated hardware logic
#include "AsyncDebug.h"

/* Hardware Configuration - Consistently defined here for hardware baseline */
#define LOCONET_PIN_RX 22
#define LOCONET_PIN_TX 23

// THE DEFINITIONS - These exist exactly once in the project after merger
LocoNetBus bus;
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1, LOCONET_PIN_RX, LOCONET_PIN_TX, false, true, &bus);

QueueHandle_t lnToNetQueue;
QueueHandle_t netToLnQueue;

void setup() {
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) { delay(10); }

    // Initialize the new Async Debugger
    Debug::begin();
 
    // Increased queue depth slightly to handle bursty LocoNet traffic
    lnToNetQueue = xQueueCreate(32, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(32, sizeof(lnMsg));

    lnStream.start(); // Restored from original hardware baseline
    parser.onPacket(CALLBACK_FOR_ALL_OPCODES, [](const lnMsg *p) {
        xQueueSend(lnToNetQueue, p, 0); 
    });
    xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);
    LOG_DEBUG("%s initialized\n", BRIDGE_VERSION);
}

void loop() {
    lnStream.process();

    static lnMsg tx;
    if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
        // 1. Send to hardware
        lnStream.send(&tx);

        // 2. Manual Echo
        // We push a copy back to the RX queue so all clients see the confirmation.
        // If this causes double-messages later, we know the hardware is echoing.
        xQueueSend(lnToNetQueue, &tx, 0); 
    }
}
