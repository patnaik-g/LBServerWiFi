#include "NetworkInterface.h"
#include "AsyncDebug.h"

uint32_t lastTrafficMilli = 0;
void communicationTask(void *pvParameters) {
    static ProtocolBuffer inbound;
    static char out[128];
    static char rsp_ok[] = "SENT OK\r\n";
    const uint32_t SIG_SEND = 0x444E4553; 

    out[0] = 'R'; out[1] = 'E'; out[2] = 'C'; out[3] = 'E';
    out[4] = 'I'; out[5] = 'V'; out[6] = 'E';

    for (;;) {
        uint32_t now = millis();
        wifiManager.checkNewConnections();
        bool anyActive = false;
        
        wifiManager.forEachActiveClient([&](WiFiClient& client, int i) {
            anyActive = true;
            while (client.available() > 0) {
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
                    if (txIdx > 0 && g_SystemPower) {
                        if (xQueueSend(netToLnQueue, &tx, 0) == pdPASS) {
                            client.write(rsp_ok, 9);
                            digitalWrite(PIN_STATUS_LED, HIGH);
                            lastTrafficMilli = now;
                            LOG_DEBUG("RX[%d]: %s\n", i, inbound.asChars);
                        }
                    }
                }
            }
        });

        lnMsg rx;
        if (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
            if (g_SystemPower) {
                uint8_t pktLen = getPacketLen(&rx);
                static const char hex[] = "0123456789ABCDEF";
                int pos = 7;
                for (uint8_t i = 0; i < pktLen; i++) {
                    out[pos++] = ' ';
                    out[pos++] = hex[(rx.data[i] >> 4) & 0x0F];
                    out[pos++] = hex[rx.data[i] & 0x0F];
                }
                out[pos++] = '\r'; 
                out[pos++] = '\n';
                
                if (anyActive) {
                    wifiManager.broadcast(out, pos);
                    digitalWrite(PIN_STATUS_LED, HIGH);
                    lastTrafficMilli = now;
                }

                // Explicit logging of incoming traffic to console
                out[pos] = '\0';
                LOG_DEBUG("%s", out);
            }
            // If !g_SystemPower, packet is popped and naturally discarded
        }

        wifiManager.loopMaintenance(anyActive);
        vTaskDelay(pdMS_TO_TICKS(g_SystemPower ? 1 : 20));
    }
}
