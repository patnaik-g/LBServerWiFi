#include "NetworkInterface.h"
#include "AsyncDebug.h"
#include "PowerLine.h"

extern PowerLine powerMonitor;

void communicationTask(void *pvParameters) {
    wifiManager.begin();
    pinMode(PIN_STATUS_LED, OUTPUT);
    
    ProtocolBuffer inbound; 
    char out[128];
    char rsp_ok[10];
    
    // Pre-calculate fixed strings
    out[0] = 'R'; out[1] = 'E'; out[2] = 'C';
    out[3] = 'E'; out[4] = 'I'; out[5] = 'V'; out[6] = 'E';
    
    rsp_ok[0] = 'S'; rsp_ok[1] = 'E';
    rsp_ok[2] = 'N'; rsp_ok[3] = 'T';
    rsp_ok[4] = ' '; rsp_ok[5] = 'O'; rsp_ok[6] = 'K'; 
    rsp_ok[7] = '\r';
    rsp_ok[8] = '\n'; rsp_ok[9] = '\0';
    
    const uint32_t SIG_SEND = 0x444E4553; // "SEND"

    for (;;) {
        wifiManager.checkNewConnections();

        bool anyActive = false;
        wifiManager.forEachActiveClient([&](WiFiClient& client, int i) {
            anyActive = true;
            
            while (client.available() > 0) {
                size_t len = client.readBytesUntil('\n', inbound.asChars, 255);
                
                // GATE: Check Global Variable
                if (g_PowerState) {
                    if (len >= 4 && *(uint32_t*)inbound.asChars == SIG_SEND) {
                        inbound.asChars[len] = '\0';
                        LOG_DEBUG("RX[%d]: %s\n", i, inbound.asChars);
                        
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
                            client.write(rsp_ok, 9);
                            LOG_DEBUG("Ack: %s", rsp_ok);
                        }
                    }
                }
            }
        });

        wifiManager.loopMaintenance(anyActive);

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
            
            out[pos] = '\0';
            LOG_DEBUG("BCAST: %s", out);
        }
        vTaskDelay(1);
    }
}
