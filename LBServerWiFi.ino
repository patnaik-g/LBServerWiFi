/**
 * LBServerWiFi v2.4.0 - Main Orchestrator
 */

#include "Config.h"
#include "NetworkInterface.h"
#include "AsyncDebug.h"
#include "PowerLine.h"
#include "ActivityMonitor.h"
#ifdef SYSTEM_POWER_CONTROL
#include "KasaSmartPlug.h"
#endif

LocoNetBus bus;
LocoNetDispatcher parser(&bus);
// Updated to use the consistent PIN_ naming convention
LocoNetStreamESP32 lnStream(1, PIN_LOCONET_RX, PIN_LOCONET_TX, false, true, &bus);

// Initialize to safe defaults
volatile bool g_SystemPower = false;
volatile bool g_TrackPower = false;
// Handles are initialized in setup(), so NULL here is fine
QueueHandle_t lnToNetQueue = NULL;
QueueHandle_t netToLnQueue = NULL;

// Use Config.h Defaults
WiFiManager wifiManager(DEFAULT_HOSTNAME, DEFAULT_PORT);
PowerLine powerMonitor;

#ifdef SYSTEM_POWER_CONTROL
ActivityMonitor watchdog(TIMEOUT_TRACK_MS, TIMEOUT_SYSTEM_MS);
KasaPlug* systemPlug = NULL;
#else
ActivityMonitor watchdog(TIMEOUT_TRACK_MS);
#endif

void setup() {
    btStop();
    Serial.begin(SERIAL_BAUD_RATE);
    pinMode(PIN_STATUS_LED, OUTPUT);

    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) { delay(10); }
    
    Debug::begin();
    // Now using Config.h constant for Queue Depth
    lnToNetQueue = xQueueCreate(LOCONET_QUEUE_DEPTH, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(LOCONET_QUEUE_DEPTH, sizeof(lnMsg));
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

#ifdef SYSTEM_POWER_CONTROL
    // Hardware Discovery
    LOG_DEBUG("Scanning for Kasa Plug '%s'...\n", KASA_SMARTPLUG_NAME);
    systemPlug = KasaPlug::Find(KASA_SMARTPLUG_NAME);
    if (systemPlug) {
        LOG_DEBUG("Kasa Plug Found: %s\n", systemPlug->ip);
    } else {
        LOG_DEBUG("Kasa Plug Not Found.\n");
    }
    
    // Inject Dependencies
    watchdog.begin(&lnStream, lnToNetQueue, systemPlug);
#else
    watchdog.begin(&lnStream, lnToNetQueue);
#endif

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
