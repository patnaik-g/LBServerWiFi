/**
 * LBServerWiFi v2.0.0 - Main Orchestrator
 */

#include "NetworkInterface.h"
#include <LocoNetStreamESP32.h>
#include "AsyncDebug.h"
#include "PowerLine.h"

#define LOCONET_PIN_RX 22
#define LOCONET_PIN_TX 23
#define PIN_POWER_MONITOR 34

LocoNetBus bus;
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1, LOCONET_PIN_RX, LOCONET_PIN_TX, false, true, &bus);
QueueHandle_t lnToNetQueue;
QueueHandle_t netToLnQueue;
// Hostname updated to lowercase "lbserver" to ensure reliable mDNS discovery
WiFiManager wifiManager("lbserver", 1234);
PowerLine powerMonitor;

void setup() {
    btStop();
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) { delay(10); }
    
    Debug::begin();

    lnToNetQueue = xQueueCreate(32, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(32, sizeof(lnMsg));
    
    lnStream.start();
    parser.onPacket(CALLBACK_FOR_ALL_OPCODES, [](const lnMsg *p) {
        xQueueSend(lnToNetQueue, p, 0); 
    });

    xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);

    powerMonitor.begin(PIN_POWER_MONITOR);

    LOG_DEBUG("%s initialized\n", BRIDGE_VERSION);
}

void loop() {
    lnStream.process();

    static lnMsg tx;
    if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
        // Gated Execution: Check Global Variable
        if (g_PowerState) {
            lnStream.send(&tx);
            xQueueSend(lnToNetQueue, &tx, 0); 
        }
    }
}
