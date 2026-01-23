#include "NetworkInterface.h"
#include "LoconetInterface.h"
#include "debug.h"

WiFiManager wifiManager("LBServer");
WiFiClient &netClient = wifiManager.getClient();

/**
 * fastHexToByte
 * Optimized for Uppercase ASCII hex conversion.
 */
static inline uint8_t fastHexToByte(char high, char low) {
    auto toN = [](char c) -> uint8_t {
        if (c >= 'A') return c - 'A' + 10;
        return c - '0';
    };
    return (toN(high) << 4) | (toN(low) & 0x0F);
}

void communicationTask(void *pvParameters) {
    wifiManager.begin();
    pinMode(PIN_STATUS_LED, OUTPUT);
    
    char buf[256];
    char out[128]; 
    char cmd_send[4]; 
    char rsp_ok[10]; 
    
    // --- ONE-TIME INITIALIZATION ---
    out[0] = 'R'; out[1] = 'E'; out[2] = 'C'; out[3] = 'E';
    out[4] = 'I'; out[5] = 'V'; out[6] = 'E';
    
    cmd_send[0] = 'S'; cmd_send[1] = 'E'; cmd_send[2] = 'N'; cmd_send[3] = 'D';
    
    rsp_ok[0] = 'S'; rsp_ok[1] = 'E'; rsp_ok[2] = 'N'; rsp_ok[3] = 'T';
    rsp_ok[4] = ' '; rsp_ok[5] = 'O'; rsp_ok[6] = 'K'; 
    rsp_ok[7] = '\r'; rsp_ok[8] = '\n'; rsp_ok[9] = '\0';
    
    bool lastNetConnected = false;
    unsigned long lastBlink = 0;
    bool ledState = false;

    for (;;) {
        bool currentNetConnected = netClient.connected();

        // Connection & Blinking
        if (!currentNetConnected) {
            netClient = wifiManager.getServer().available();
            currentNetConnected = netClient.connected();
            if (!currentNetConnected && (millis() - lastBlink > 500)) {
                ledState = !ledState;
                digitalWrite(PIN_STATUS_LED, ledState);
                lastBlink = millis();
            }
        }

        if (currentNetConnected && !lastNetConnected) {
            netClient.write("VERSION ESP32 LocoNet Bridge v1.7.0\r\n", 38);
            LOG_DEBUG(">>> JMRI Client Connected\n");
            digitalWrite(PIN_STATUS_LED, HIGH);
            lastNetConnected = true;
        } else if (!currentNetConnected) {
            lastNetConnected = false;
        }

        // Inbound (Prioritized Drain)
        if (currentNetConnected) {
            while (netClient.available() > 0) {
                size_t len = netClient.readBytesUntil('\n', buf, sizeof(buf) - 1);
                if (len >= 4 && 
                    buf[0] == cmd_send[0] && buf[1] == cmd_send[1] && 
                    buf[2] == cmd_send[2] && buf[3] == cmd_send[3]) {
                    
                    buf[len] = '\0';
                    LOG_DEBUG("DEBUG: %s\n", buf);
                    
                    lnMsg tx; uint8_t txIdx = 0;
                    char *p = buf + 4;

                    while (p < (buf + len) && txIdx < sizeof(tx.data)) {
                        if (*p <= 32) { p++; continue; } 
                        if (*p && *(p + 1)) {
                            tx.data[txIdx++] = fastHexToByte(*p, *(p + 1));
                            p += 2;
                        } else p++;
                    }
                    if (txIdx > 0 && xQueueSend(netToLnQueue, &tx, 0) == pdPASS) {
                        netClient.write(rsp_ok, 9);
                        LOG_DEBUG("DEBUG: %s", rsp_ok);
                    }
                }
            }
        }

        // Outbound (Binary to ASCII)
        lnMsg rx;
        if (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
            uint8_t pktLen = getPacketLen(&rx);
            static const char hex[] = "0123456789ABCDEF";
            
            int pos = 7; 
            for (uint8_t i = 0; i < pktLen; i++) {
                out[pos++] = ' ';
                out[pos++] = hex[(rx.data[i] >> 4) & 0x0F];
                out[pos++] = hex[rx.data[i] & 0x0F];
            }
            out[pos++] = '\r'; out[pos++] = '\n';
            if (currentNetConnected) netClient.write(out, pos);
            out[pos] = '\0';
            LOG_DEBUG("DEBUG: %s", out);
        }
        vTaskDelay(1);
    }
}