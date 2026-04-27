/**
 * LBServerWiFi - Main Orchestrator
 */

#include "Config.h"
#include "NetworkInterface.h"
#include "AsyncDebug.h"
#ifdef ENABLE_POWER_MONITOR
#include "PowerLine.h"
#endif
#include "ActivityMonitor.h"
#ifdef ENABLE_KASA_CONTROL
#include "KasaSmartPlug.h"
#endif
#include "UartTuning.h"

LocoNetBus bus;
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1, PIN_LOCONET_RX, PIN_LOCONET_TX, false, true, &bus);

#ifdef ENABLE_POWER_MONITOR
volatile bool g_SystemPower = false;
PowerLine powerMonitor;
#endif

volatile bool g_TrackPower = false;
uint32_t lastTrafficMilli = 0;

QueueHandle_t lnToNetQueue = NULL;
QueueHandle_t netToLnQueue = NULL;

WiFiManager wifiManager(DEFAULT_HOSTNAME, DEFAULT_PORT);

#ifdef ENABLE_KASA_CONTROL
ActivityMonitor watchdog(TIMEOUT_TRACK_MS, TIMEOUT_SYSTEM_MS);
KasaPlug* systemPlug = NULL;
#else
ActivityMonitor watchdog(TIMEOUT_TRACK_MS);
#endif

void onPacket(const lnMsg* p) {
  if (p->data[0] == 0x81 && p->data[1] == 0x7E) return;
  digitalWrite(PIN_STATUS_LED, HIGH);
  lastTrafficMilli = millis();
  xQueueSend(lnToNetQueue, p, 0);
}

void setup() {
  btStop();
#ifdef ENABLE_SERIAL_LOGGING
  Serial.begin(SERIAL_BAUD_RATE);
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) { vTaskDelay(pdMS_TO_TICKS(10)); }
#endif

  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_LOCONET_RX, INPUT);

  Debug::begin();
  
  lnToNetQueue = xQueueCreate(LOCONET_QUEUE_DEPTH, sizeof(lnMsg));
  netToLnQueue = xQueueCreate(LOCONET_QUEUE_DEPTH, sizeof(lnMsg));

  wifiManager.begin();
  xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);

  parser.onPacket(CALLBACK_FOR_ALL_OPCODES, onPacket);
  lnStream.start();
  applyUartTuning();

  watchdog.reset();

#ifdef ENABLE_KASA_CONTROL
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

#ifdef ENABLE_POWER_MONITOR
  powerMonitor.begin(PIN_POWER_MONITOR, &lnStream);
#endif

  LOG_DEBUG("%s initialized\n", BRIDGE_VERSION);
}

void loop() {
  checkUartErrors();

  if (g_SystemPower) {
    lnStream.process();
  } else {
    // When system power is off, we must NOT process the physical LocoNet bus,
    // as it can be electrically noisy. However, we MUST continue to process
    // the netToLnQueue to handle client commands and spoof responses.
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  static lnMsg tx;
  if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
    if (g_SystemPower) {
      lnStream.send(&tx);
      digitalWrite(PIN_STATUS_LED, HIGH);
      lastTrafficMilli = millis();
    }
    LOG_DEBUG("Local echo, OpCode %02X\n", tx.data[0]);
    xQueueSend(lnToNetQueue, &tx, 0);
  }

  // If there has been no traffic for a short period, yield to other tasks.
  // This keeps the loop "hot" during packet bursts but prevents it from
  // consuming 100% of the core when the bus is idle.
  const uint32_t BUSY_WINDOW_MS = 10;
  if (millis() - lastTrafficMilli > BUSY_WINDOW_MS) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
