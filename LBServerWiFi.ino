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
#include "soc/uart_struct.h"
#include "soc/uart_reg.h"

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

// Pre-calculated mask for the hardware watchdog
const uint32_t WATCHDOG_ERROR_MASK = (
    UART_RXFIFO_OVF_INT_RAW_M    | 
    UART_FRM_ERR_INT_RAW_M       | 
    UART_PARITY_ERR_INT_RAW_M    | 
    UART_GLITCH_DET_INT_RAW_M    |
    UART_BRK_DET_INT_RAW_M         // Added - detect long power off condition
);
void onPacket(const lnMsg *p) {
    if (p->data[0] == 0x81 && p->data[1] == 0x7E) return;
    digitalWrite(PIN_STATUS_LED, HIGH);
    lastTrafficMilli = millis();
    watchdog.inspect(p);
    
    if (lnToNetQueue) {
        xQueueSend(lnToNetQueue, p, 0);
    }
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

    // Hardware Glitch Filter configuration
    uint32_t conf0 = READ_PERI_REG(UART_CONF0_REG(1));
    conf0 &= ~(0xFF << UART_GLITCH_FILT_S);
    conf0 |= (0xFF << UART_GLITCH_FILT_S);
    WRITE_PERI_REG(UART_CONF0_REG(1), conf0);
    
    Debug::begin();
    lnToNetQueue = xQueueCreate(LOCONET_QUEUE_DEPTH, sizeof(lnMsg));
    netToLnQueue = xQueueCreate(LOCONET_QUEUE_DEPTH, sizeof(lnMsg));

    wifiManager.begin();
    xTaskCreatePinnedToCore(communicationTask, "Comm", 4096, NULL, 1, NULL, 0);
    
    parser.onPacket(CALLBACK_FOR_ALL_OPCODES, onPacket);
    lnStream.start();
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
  
volatile uint32_t uart_errs = UART1.int_raw.val;
    
    if (uart_errs & WATCHDOG_ERROR_MASK) {
        LOG_DEBUG("WATCHDOG: Triggered by[%s%s%s%s%s]\n", 
	      (uart_errs & UART_PARITY_ERR_INT_RAW_M)  ? " PARITY"  : "",
          (uart_errs & UART_FRM_ERR_INT_RAW_M)     ? " FRAME"   : "",
          (uart_errs & UART_RXFIFO_OVF_INT_RAW_M)  ? " OVF"     : "",
          (uart_errs & UART_GLITCH_DET_INT_RAW_M)  ? " GLITCH"  : "",
          (uart_errs & UART_BRK_DET_INT_RAW_M)     ? " BREAK"   : "");
        UART1.int_clr.val = WATCHDOG_ERROR_MASK;
    }
   // --- WATCHDOG: Event-Driven Reporting ---
#ifdef ENABLE_POWER_MONITOR
    if (watchdog.isSystemOff()) {
        vTaskDelay(pdMS_TO_TICKS(250));
        return;
    }
#endif

    lnStream.process();

    static lnMsg tx;
    bool worked = false;
    if (xQueueReceive(netToLnQueue, &tx, 0) == pdPASS) {
        lnStream.send(&tx);
        digitalWrite(PIN_STATUS_LED, HIGH);
        lastTrafficMilli = millis();
        LOG_DEBUG("Local echo, OpCode %02X\n", tx.data[0]);
        xQueueSend(lnToNetQueue, &tx, 0);
        watchdog.inspect(&tx);
        worked = true;
    }

    watchdog.manage();
    if (!worked) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
