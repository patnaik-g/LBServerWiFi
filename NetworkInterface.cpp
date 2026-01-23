#include "NetworkInterface.h"
#include "AsyncDebug.h"

WiFiManager wifiManager("LBServer");
WiFiClient &netClient = wifiManager.getClient();

/**
 * Protocol Buffer Union
 * Forces 4-byte alignment to allow safe 32-bit integer comparisons 
 */
union ProtocolBuffer {
    char asChars[256];
    uint32_t asUint32[64]; 
};

/**
 * fastHexToByte
 * Optimized for Uppercase ASCII hex conversion (0-9, A-F).
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
    
    ProtocolBuffer inbound; 
    char out[128]; 
    char rsp_ok[10]; 
    
    // --- ONE-TIME INITIALIZATION ---
    out[0] = 'R'; out[1] = 'E'; out[2] = 'C'; out[3] = 'E';
    out[4] = 'I'; out[5] = 'V'; out[6] = 'E';
    
    rsp_ok[0] = 'S'; rsp_ok[1] = 'E'; rsp_ok[2] = 'N'; rsp_ok[3] = 'T';
    rsp_ok[4] = ' '; rsp_ok[5] = 'O'; rsp_ok[6] = 'K'; 
    rsp_ok[7] = '\r'; rsp_ok[8] = '\n'; rsp_ok[9] = '\0';
    
    const uint32_t SIG_SEND = 0x444E4553; 
    
    bool lastNetConnected = false;
    unsigned long lastBlink = 0;
    bool ledState = false;

    for (;;) {
        bool currentNetConnected = netClient.connected();

        // --- SECTION 1: Connection & Blinking ---
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
            // OPTIMIZATION: Disable Nagle for real-time control
            netClient.setNoDelay(true); 
            // OPTIMIZATION: Prevent blocking outbound traffic
            netClient.setTimeout(10);   
            
            netClient.write("VERSION ESP32 LocoNet Bridge v1.7.5\r\n", 38);
            LOG_DEBUG(">>> JMRI Client Connected\n");
            digitalWrite(PIN_STATUS_LED, HIGH);
            lastNetConnected = true;
        } else if (!currentNetConnected) {
            lastNetConnected = false;
        }

        // --- SECTION 2: Inbound ---
        if (currentNetConnected) {
            while (netClient.available() > 0) {
                // Now uses the 10ms timeout to avoid locking the task
                size_t len = netClient.readBytesUntil('\n', inbound.asChars, 255);
                
                if (len >= 4 && *(uint32_t*)inbound.asChars == SIG_SEND) {
                    
                    inbound.asChars[len] = '\0';
                    LOG_DEBUG("DEBUG: %s\n", inbound.asChars);
                    
                    netClient.write(out, 7);              
                    netClient.write(inbound.asChars + 4, len - 4); 
                    netClient.write("\r\n", 2);

                    lnMsg tx; uint8_t txIdx = 0;
                    char *p = inbound.asChars + 4;

                    while (p < (inbound.asChars + len) && txIdx < sizeof(tx.data)) {
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

        // --- SECTION 3: Outbound ---
        lnMsg rx;
        if (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
            // getPacketLen is now linked from the header
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
