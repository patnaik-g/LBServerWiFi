/**
 * LBServerWiFi v2.3.0 - Main Orchestrator
 */

#include "NetworkInterface.h"
#include <LocoNetStreamESP32.h>
#include "AsyncDebug.h"
#include "PowerLine.h"
#include "ActivityMonitor.h"
#include "KasaSmartPlug.h"

#define LOCONET_PIN_RX 22
#define LOCONET_PIN_TX 23
#define PIN_POWER_MONITOR 34

#define TIMEOUT_TRACK_MS 60000   // 15 Minutes
#define TIMEOUT_SYSTEM_MS 180000 // 30 Minutes

LocoNetBus bus;
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1, LOCONET_PIN_RX, LOCONET_PIN_TX, false, true, &bus);

QueueHandle_t lnToNetQueue;
QueueHandle_t netToLnQueue;

WiFiManager wifiManager("lbserver", 1234);
PowerLine powerMonitor;
ActivityMonitor watchdog(TIMEOUT_TRACK_MS, TIMEOUT_SYSTEM_MS);
KasaPlug* systemPlug = NULL;

void setup() {
    btStop();
    Serial.begin(115200);
    pinMode(PIN_STATUS_LED, OUTPUT);

    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) { delay(10); }
    
    Debug::begin();

    lnToNetQueue = xQueueCreate(32, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(32, sizeof(lnMsg));

    // Blocking Network Init
    wifiManager.begin();
    xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);
    
    // Start Logic
    parser.onPacket(CALLBACK_FOR_ALL_OPCODES, [](const lnMsg *p) {
        watchdog.inspect(p);
        xQueueSend(lnToNetQueue, p, 0); 
    });

    lnStream.start();
    watchdog.reset();

    // Hardware Discovery
    LOG_DEBUG("Scanning for Kasa Plug 'Layout'...\n");
    systemPlug = KasaPlug::Find("Layout");
    if (systemPlug) {
        LOG_DEBUG("Kasa Plug Found: %s\n", systemPlug->ip);
    } else {
        LOG_DEBUG("Kasa Plug Not Found.\n");
    }
    
    // Inject Dependencies
    watchdog.begin(&lnStream, lnToNetQueue, systemPlug);
    powerMonitor.begin(PIN_POWER_MONITOR);

    LOG_DEBUG("%s initialized\n", BRIDGE_VERSION);
}

void loop() {
    // 1. Hardware Gate (State tracked inside watchdog)
    if (watchdog.isSystemOff()) {
        delay(10); 
        return;
    }

    // 2. Core Logic
    lnStream.process();

    // 3. Bridge: WiFi -> LocoNet
    static lnMsg tx;
    if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
        lnStream.send(&tx);
        xQueueSend(lnToNetQueue, &tx, 0);
        watchdog.inspect(&tx);
    }

    // 4. Housekeeping
    watchdog.manage();
}
