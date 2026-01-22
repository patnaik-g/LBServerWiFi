#include "NetworkInterface.h"
#include "LoconetInterface.h"
#include "debug.h"

WiFiManager wifiManager("LBServer");
WiFiClient &netClient = wifiManager.getClient();

// Static lookup table for high-speed hex conversion
static const char hexTable[] = "0123456789ABCDEF";

/**
 * communicationTask: Manages the bidirectional bridge between the WiFi network
 * (LBServer protocol) and the internal LocoNet queues. It handles client 
 * persistence, command parsing, and layout status reporting.
 */
void communicationTask(void *pvParameters) {
    wifiManager.begin();
    pinMode(PIN_STATUS_LED, OUTPUT);
    
    static char buf[64];  // Inbound network buffer
    static char out[128]; // Outbound network buffer
    bool lastNetConnected = false;

    for (;;) {
        bool currentNetConnected = netClient.connected();
        
        // --- SECTION 1: CONNECTION MANAGEMENT ---
        // Handles new client connections, version handshaking, and status LED blinking
        if (!currentNetConnected) {
            netClient = wifiManager.getServer().available();
            currentNetConnected = netClient.connected();
            if (currentNetConnected && !lastNetConnected) {
                netClient.println("VERSION ESP32 LocoNet Bridge v1.6.4");
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

        // --- SECTION 2: INBOUND (JMRI -> LOCONET) ---
        // Drains network buffer, trims whitespace/CRLF, and queues commands for the hardware
        while (netClient.available() > 0) {
            size_t length = netClient.readBytesUntil('\n', buf, sizeof(buf) - 1);
            if (length > 0) {
                buf[length] = '\0';
                
                // Trim trailing \r or spaces (ASCII <= 32)
                while (length > 0 && (buf[length-1] <= 32)) {
                    buf[--length] = '\0';
                }

                if (length > 0) {
                    LOG_DEBUG("DEBUG: %s\n", buf);
                    
                    if (processLbServerCommand(buf, netToLnQueue)) {
                        netClient.print("SENT OK\r\n");
                        netClient.flush();
                        LOG_DEBUG("DEBUG: SENT OK\n");
                    } else {
                        LOG_DEBUG("DEBUG: Parser REJECTED [%s]\n", buf);
                    }
                }
            }
        }

        // --- SECTION 3: OUTBOUND (LOCONET -> JMRI) ---
        // Dequeues hardware messages, assembles them into strings, and transmits to network
        lnMsg rx;
        if (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
            uint8_t len = getPacketLen(&rx);
            
            // Assemble the "RECEIVE" string manually in RAM for atomic transmission
            memcpy(out, "RECEIVE", 7);
            int pos = 7;
            for (uint8_t i = 0; i < len; i++) {
                out[pos++] = ' ';
                out[pos++] = hexTable[(rx.data[i] >> 4) & 0x0F];
                out[pos++] = hexTable[rx.data[i] & 0x0F];
            }
            out[pos++] = '\r'; 
            out[pos++] = '\n';
            
            if (currentNetConnected) {
                netClient.write((const uint8_t*)out, pos);
                netClient.flush();
            }
            
            // Mirror exactly what was sent to the Debug Log
            out[pos] = '\0'; 
            LOG_DEBUG("DEBUG: %s", out); 
        }

        vTaskDelay(1); 
    }
}
