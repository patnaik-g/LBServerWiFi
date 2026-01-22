#include "NetworkInterface.h"
#include "LoconetInterface.h"
#include "debug.h"

WiFiManager wifiManager("LBServer");
WiFiClient &netClient = wifiManager.getClient();

// Static lookup table for high-speed hex conversion
static const char hexTable[] = "0123456789ABCDEF";

void communicationTask(void *pvParameters) {
    wifiManager.begin();
    pinMode(PIN_STATUS_LED, OUTPUT);
    
    char buf[64]; 
    int idx = 0;
    bool lastNetConnected = false;

    for (;;) {
        bool currentNetConnected = netClient.connected();
        
        // 1. Connection Management
        if (!currentNetConnected) {
            netClient = wifiManager.getServer().available();
            currentNetConnected = netClient.connected();
            if (currentNetConnected && !lastNetConnected) {
                netClient.println("VERSION ESP32 LocoNet Bridge v1.6.2");
                LOG_DEBUG(">>> JMRI Client Connected\n");
                digitalWrite(PIN_STATUS_LED, HIGH);
                lastNetConnected = true;
            } else {
                static unsigned long lastBlink = 0;
                static bool ledState = false;
                ArduinoOTA.handle();
                if (millis() - lastBlink > 500) {
                    ledState = !ledState;
                    digitalWrite(PIN_STATUS_LED, ledState);
                    lastBlink = millis();
                }
                lastNetConnected = false;
                vTaskDelay(10); 
                continue; 
            }
        }

        // 2. Inbound Path: JMRI -> LocoNet (Verified Logic)
        while (netClient.available() > 0) {
            char c = netClient.read();
            if (c == '\n' || c == '\r') {
                if (idx > 0) {
                    buf[idx] = '\0';
                    LOG_DEBUG("DEBUG: Received [%s]\n", buf);
                    if (processLbServerCommand(buf, netToLnQueue)) {
                        netClient.println("SENT OK");
                        netClient.flush();
                        LOG_DEBUG("DEBUG: SENT OK\n");
                    } else {
                        LOG_DEBUG("DEBUG: Parser REJECTED command\n");
                    }
                    idx = 0;
                }
            } else if (idx < sizeof(buf) - 1) {
                buf[idx++] = c;
            }
        }

        // 3. Outbound Path: LocoNet -> JMRI (Optimized Hex Conversion)
        lnMsg rx;
        if (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
            uint8_t len = getPacketLen(&rx);
            
            // Network Output: Efficient hex construction
            netClient.print("RECEIVE");
            for (uint8_t i = 0; i < len; i++) {
                netClient.print(' ');
                netClient.print(hexTable[(rx.data[i] >> 4) & 0x0F]);
                netClient.print(hexTable[rx.data[i] & 0x0F]);
            }
            netClient.println();
            netClient.flush();

            // Debug Mirror: Matching logic
            LOG_DEBUG("DEBUG: RECEIVE");
            for (uint8_t i = 0; i < len; i++) {
                LOG_DEBUG(" %c%c", hexTable[(rx.data[i] >> 4) & 0x0F], hexTable[rx.data[i] & 0x0F]);
            }
            LOG_DEBUG("\n");
        }

        vTaskDelay(1); 
    }
}
