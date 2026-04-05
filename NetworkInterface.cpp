#include "NetworkInterface.h"
#include "AsyncDebug.h"

void communicationTask(void *pvParameters) {
    static ProtocolBuffer inbound;
    static char out[128];
    static char rsp_ok[] = "SENT OK\r\n";
    const uint32_t SIG_SEND = 0x444E4553; 

    out[0] = 'R'; out[1] = 'E';
    out[2] = 'C'; out[3] = 'E';
    out[4] = 'I'; out[5] = 'V'; out[6] = 'E';
    for (;;) {
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
                    if (txIdx > 0) {
                        if (validateChecksum(&tx)) {
                            client.write(rsp_ok, 9);
#ifdef SYSTEM_POWER_CONTROL
                            if (g_SystemPower) {
#endif
                                if (xQueueSend(netToLnQueue, &tx, 0) == pdPASS) {
                                    LOG_DEBUG("RX[%d]: %s\n", i, inbound.asChars);
                                }
#ifdef SYSTEM_POWER_CONTROL
                            } else {
                                // Power Off: Echo and Spoof Response
                                xQueueSend(lnToNetQueue, &tx, 0); 
                                
                                if (tx.data[0] == 0xBB) {
                                    // Spoof Slot Data Response (0xE7) for Address Request
                                    lnMsg slotRsp;
                                    uint8_t raw[] = { 
                                        0xE7, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 
                                        0x06, 0x00, 0x00, 0x00, 0x00, 0x00 
                                    };
                                    memcpy(slotRsp.data, raw, 13);
                                    
                                    uint8_t chk = 0;
                                    for(int j=0; j<13; j++) chk ^= slotRsp.data[j];
                                    slotRsp.data[13] = (uint8_t)(chk ^ 0xFF);
                                    xQueueSend(lnToNetQueue, &slotRsp, 0);
                                    LOG_DEBUG("RX[%d]: %s (Spoofed E7 - Power Off)\n", i, inbound.asChars);
                                } else if (tx.data[0] != 0x82 && tx.data[0] != 0x83) {
                                    // Spoof Long Ack (0xB4) for other commands
                                    lnMsg ack;
                                    ack.data[0] = 0xB4;
                                    ack.data[1] = tx.data[0] & 0x7F;
                                    ack.data[2] = 0x00;
                                    uint8_t chk = ack.data[0] ^ ack.data[1] ^ ack.data[2];
                                    ack.data[3] = (uint8_t)(chk ^ 0xFF);
                                    
                                    xQueueSend(lnToNetQueue, &ack, 0);
                                    LOG_DEBUG("RX[%d]: %s (Spoofed B4 - Power Off)\n", i, inbound.asChars);
                                }
                            }
#endif
                        } else {
                            LOG_DEBUG("CRC FAIL: %s\n", inbound.asChars);
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
            out[pos++] = '\r';
            out[pos++] = '\n';
            if (anyActive) {
                wifiManager.broadcast(out, pos);
            }
            out[pos] = '\0';
            LOG_DEBUG("%s", out);
        }

        wifiManager.loopMaintenance(anyActive);
#ifdef SYSTEM_POWER_CONTROL
        vTaskDelay(pdMS_TO_TICKS(g_SystemPower ? 1 : 20));
#else
        vTaskDelay(pdMS_TO_TICKS(1));
#endif
    }
}
