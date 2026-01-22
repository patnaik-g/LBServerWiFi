#include "NetworkInterface.h"
#include "LoconetInterface.h"
#include "debug.h"

WiFiManager wifiManager("LBServer");
WiFiClient &netClient = wifiManager.getClient();

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
                netClient.println("VERSION ESP32 LocoNet Bridge v1.6.1");
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

        // 2. Inbound Path: JMRI -> LocoNet
        while (netClient.available() > 0) {
            char c = netClient.read();
            if (c == '\n' || c == '\r') {
                if (idx > 0) {
                    buf[idx] = '\0';
                    LOG_DEBUG("DEBUG: Received [%s]\n", buf);
                    
                    if (processLbServerCommand(buf, netToLnQueue)) {
                        netClient.println("SENT OK");
                        netClient.flush();        // Force acknowledgment to JMRI
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

        // 3. Outbound Path: LocoNet -> JMRI
        lnMsg rx;
        if (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
            uint8_t len = getPacketLen(&rx);
            
            // Network Output
            netClient.print("RECEIVE");
            for (uint8_t i = 0; i < len; i++) {
                netClient.printf(" %02X", rx.data[i]);
            }
            netClient.println();
            netClient.flush(); // Ensure JMRI sees layout updates instantly

            // Debug Mirror
            LOG_DEBUG("DEBUG: RECEIVE");
            for (uint8_t i = 0; i < len; i++) {
                LOG_DEBUG(" %02X", rx.data[i]);
            }
            LOG_DEBUG("\n");
        }

        vTaskDelay(1); 
    }
}
