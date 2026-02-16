/**
 * LBServerWiFi v2.5.3 - Main Orchestrator
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
LocoNetStreamESP32 lnStream(1, PIN_LOCONET_RX, PIN_LOCONET_TX, false, true, &bus);

volatile bool g_SystemPower = false;
volatile bool g_TrackPower = false;
QueueHandle_t lnToNetQueue = NULL;
QueueHandle_t netToLnQueue = NULL;

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

    #ifdef ENABLE_SERIAL_LOGGING
    Serial.begin(SERIAL_BAUD_RATE);
    unsigned long start = millis();
    while (!Serial && millis() - start < 2000) { delay(10); }
    #endif
    
    pinMode(PIN_STATUS_LED, OUTPUT);
    
    Debug::begin();

    lnToNetQueue = xQueueCreate(LOCONET_QUEUE_DEPTH, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(LOCONET_QUEUE_DEPTH, sizeof(lnMsg));

    wifiManager.begin();
    xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);

    parser.onPacket(CALLBACK_FOR_ALL_OPCODES, [](const lnMsg *p) {
        watchdog.inspect(p);
        xQueueSend(lnToNetQueue, p, 0); 
    });

    lnStream.start();
    watchdog.reset();

#ifdef SYSTEM_POWER_CONTROL
    LOG_DEBUG("Scanning for Kasa Plug '%s'...\n", KASA_SMARTPLUG_NAME);
    
    systemPlug = KasaPlug::Find(KASA_SMARTPLUG_NAME);

    if (systemPlug) {
        LOG_DEBUG("Kasa Plug Found: %s\n", systemPlug->ip);
    } else {
        LOG_DEBUG("Kasa Plug Not Found.\n");
    }
    
    watchdog.begin(&lnStream, lnToNetQueue, systemPlug);
#else
    watchdog.begin(&lnStream, lnToNetQueue);
#endif

    powerMonitor.begin(PIN_POWER_MONITOR);
    
    LOG_DEBUG("%s initialized\n", BRIDGE_VERSION);
}

void loop() {
    if (watchdog.isSystemOff()) {
        delay(10);
        return;
    }

    lnStream.process();

    static lnMsg tx;
    if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
        lnStream.send(&tx);
        xQueueSend(lnToNetQueue, &tx, 0);
        watchdog.inspect(&tx);
    }

    watchdog.manage();
}
