/**
 * LBServerWiFi v2.3.0 - Main Orchestrator
 */

#include "NetworkInterface.h"
#include <LocoNetStreamESP32.h>
#include "AsyncDebug.h"
#include "PowerLine.h"
#include "LocoNetPackets.h"
#include "ActivityMonitor.h"

#define LOCONET_PIN_RX 22
#define LOCONET_PIN_TX 23
#define PIN_POWER_MONITOR 34
#define IDLE_TIMEOUT_MS 900000 // 15 Minutes

LocoNetBus bus;
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1, LOCONET_PIN_RX, LOCONET_PIN_TX, false, true, &bus);
QueueHandle_t lnToNetQueue;
QueueHandle_t netToLnQueue;
WiFiManager wifiManager("lbserver", 1234);
PowerLine powerMonitor;
ActivityMonitor watchdog(IDLE_TIMEOUT_MS);

void setup() {
    btStop();
    Serial.begin(115200);
    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) { delay(10); }
    
    Debug::begin();
    lnToNetQueue = xQueueCreate(32, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(32, sizeof(lnMsg));
    
    lnStream.start();
    watchdog.reset();

    // Activity Monitor: Inspects packets to reset timer, then forwards to WiFi
    parser.onPacket(CALLBACK_FOR_ALL_OPCODES, [](const lnMsg *p) {
        watchdog.inspect(p);
        xQueueSend(lnToNetQueue, p, 0); 
    });
    
    xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);
    powerMonitor.begin(PIN_POWER_MONITOR);

    LOG_DEBUG("%s initialized\n", BRIDGE_VERSION);
}

void loop() {
    static bool wasSystemOff = false;

    // HARDWARE GATE: Check System Power (GPIO 34)
    if (!g_SystemPower) {
        wasSystemOff = true; // Mark that we are currently sleeping
        delay(10); 
        return;
    }

    // --- WAKE UP LOGIC ---
    // If we just came back from being OFF, reset the timer immediately.
    if (wasSystemOff) {
        LOG_DEBUG("System Power Restored. Resetting Idle Timer.\n");
        watchdog.reset();
        wasSystemOff = false;
    }

    // --- System is Active ---
    lnStream.process();

    // WATCHDOG: Checks g_TrackPower internally
    if (watchdog.shouldTrigger()) {
        LOG_DEBUG("Idle Timeout. Turning Track Power OFF.\n");
        lnStream.send((lnMsg*)&PACKET_GP_OFF);
        xQueueSend(lnToNetQueue, (void*)&PACKET_GP_OFF, 0);
    }

    // BRIDGE: WiFi -> LocoNet
    static lnMsg tx;
    if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
        // Since we passed the hardware gate above, it is safe to send.
        watchdog.inspect(&tx); // WiFi commands also count as activity
        lnStream.send(&tx);
        xQueueSend(lnToNetQueue, &tx, 0); 
    }
}
