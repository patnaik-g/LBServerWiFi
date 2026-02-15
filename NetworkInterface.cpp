#include "NetworkInterface.h"
#include "AsyncDebug.h"

uint32_t lastTrafficMilli = 0;

void communicationTask(void *pvParameters) {
    // STATIC ALLOCATION:
    // Moves these buffers from the Task Stack to Fixed Memory (BSS).
    // This reduces stack pressure and ensures addresses are permanent.
    static ProtocolBuffer inbound;
    static char out[128];
    static char rsp_ok[10];

    // Initialize the static buffers (runs once on task start)
    out[0] = 'R'; out[1] = 'E'; out[2] = 'C'; out[3] = 'E';
    out[4] = 'I';
    out[5] = 'V'; out[6] = 'E';
    
    rsp_ok[0] = 'S'; rsp_ok[1] = 'E'; rsp_ok[2] = 'N'; rsp_ok[3] = 'T';
    rsp_ok[4] = ' '; rsp_ok[5] = 'O'; rsp_ok[6] = 'K'; 
    rsp_ok[7] = '\r'; rsp_ok[8] = '\n'; rsp_ok[9] = '\0';
    
    const uint32_t SIG_SEND = 0x444E4553; // "SEND"

    for (;;) {
        uint32_t now = millis();
        wifiManager.checkNewConnections();
        bool anyActive = false;
        
        wifiManager.forEachActiveClient([&](WiFiClient& client, int i) {
            anyActive = true;
            
            while (client.available() > 0) {
                // Reuse the static 'inbound' buffer
                size_t len = client.readBytesUntil('\n', inbound.asChars, 255);
                
                if (len >= 4 && *(uint32_t*)inbound.asChars == SIG_SEND) {
                    inbound.asChars[len] = '\0';
                    lnMsg tx; uint8_t txIdx = 0;
                    char *p = inbound.asChars + 4;
                
                    while (p < (inbound.asChars + len) && txIdx < sizeof(tx.data)) {
                        if (*p <= 32) { p++; continue; }
                        if (*p && *(p + 1)) {
                            tx.data[txIdx++] = fastHexToByte(*p, *(p + 1));
                            p += 2;
                        } else p++;
                    }

                    if (txIdx > 0) {
                        if (g_SystemPower) {
                            if (xQueueSend(netToLnQueue, &tx, 0) == pdPASS) {
                                client.write(rsp_ok, 9);
                                digitalWrite(PIN_STATUS_LED, HIGH);
                                lastTrafficMilli = now;
                                LOG_DEBUG("RX[%d]: %s\n", i, inbound.asChars);
                                LOG_DEBUG("Ack: %s", rsp_ok);
                            }
                        } else {
                            // System Power is OFF: Silent Keep-Alive
                            if (txIdx >= 2 && tx.data[0] == 0xBB && tx.data[1] == 0x00) {
                                lnMsg s0 = { .data = { 0xE7, 0x0E, 0x00, 0x00, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x49, 0x42, 0x18 } };
                                xQueueSend(lnToNetQueue, &s0, 0);
                                client.write(rsp_ok, 9);
                                // LOGGING SUPPRESSED: Prevents heap fragmentation
                            }
                        }
                    }
                }
            }
        });

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
            wifiManager.broadcast(out, pos);
            if (anyActive) {
                digitalWrite(PIN_STATUS_LED, HIGH);
                lastTrafficMilli = now;
            }
            out[pos] = '\0';
            
            // Only log broadcast traffic if System Power is ON
            if (g_SystemPower) {
                LOG_DEBUG("BCAST: %s", out);
            }
        }

        wifiManager.loopMaintenance(anyActive);
        vTaskDelay(1);
    }
}
