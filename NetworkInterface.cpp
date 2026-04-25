#include "NetworkInterface.h"
#include "AsyncDebug.h"
#include "ActivityMonitor.h"

// These objects are defined in the main .ino file.
// Declaring them here allows this task to access them.
// This centralizes all activity-related logic to this task for thread-safety.
extern ActivityMonitor watchdog;

void communicationTask(void *pvParameters) {
  static ProtocolBuffer inbound;
  static char out[512];
  static char rsp_ok[] = "SENT OK\r\n";
  const uint32_t SIG_SEND = 0x444E4553;

  out[0] = 'R';
  out[1] = 'E';
  out[2] = 'C';
  out[3] = 'E';
  out[4] = 'I';
  out[5] = 'V';
  out[6] = 'E';
  for (;;) {
    wifiManager.checkNewConnections();
    bool anyActive = false;
    wifiManager.forEachActiveClient([&](WiFiClient &client, int i) {
      anyActive = true;
      client.setTimeout(10);
      while (client.available() > 0) {
        size_t len = client.readBytesUntil('\n', inbound.asChars, 255);
        if (len >= 4 && *(uint32_t *)inbound.asChars == SIG_SEND) {


          inbound.asChars[len] = '\0';
          lnMsg tx;
          uint8_t txIdx = 0;
  
          char *p = inbound.asChars + 4;
          while (p < (inbound.asChars + len) && txIdx < sizeof(tx.data)) {

            if (*p <= 32) {
              p++;
              continue;
            }

           
            if (*p && *(p + 1)) {
              tx.data[txIdx++] = fastHexToByte(*p, *(p + 1));

              p += 2;
            } else p++;
          }
          if (txIdx > 0) {

            if (validateChecksum(&tx)) {
      
              client.write(rsp_ok, 9);
              if (g_SystemPower) {
                if (xQueueSend(netToLnQueue, &tx, 0) == pdPASS) {
                  LOG_DEBUG("RX[%d]: %s\n", i, inbound.asChars);
                }
              } else {
                // Power Off: Echo and Spoof Response

                xQueueSend(lnToNetQueue, &tx, 0);
                if (tx.data[0] == 0xBB) {
                  if (tx.data[1] == 0x79) {
                    // Query 1: Slot 121 -> IB Long Ack

                    lnMsg ack;
                    uint8_t raw[] = { 0xB4, 0x3B, 0x00, 0x70 };
                    memcpy(ack.data, raw, 4);
                    xQueueSend(lnToNetQueue, &ack, 0);
                    LOG_DEBUG("RX[%d]: %s (Spoofed B4 - IB Status)\n", i, inbound.asChars);
                  } else if (tx.data[1] == 0x00) {
                    // Query 2: Slot 0 -> IB Identity (E7)

                    lnMsg slotRsp;
                    uint8_t raw[] = { 0xE7, 0x0E, 0x00, 0x00, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x49, 0x42, 0x19 };
                    memcpy(slotRsp.data, raw, 14);
                    xQueueSend(lnToNetQueue, &slotRsp, 0);
                    LOG_DEBUG("RX[%d]: %s (Spoofed E7 - IB Identity)\n", i, inbound.asChars);
                  }
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
            } else {
              LOG_DEBUG("CRC FAIL: %s\n", inbound.asChars);
            }
          }
        }
      }
    });
    lnMsg rx;
    while (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
      // A packet (either from LocoNet or a local echo) has been received.
      // Inspect it for activity before broadcasting.
      watchdog.inspect(&rx);
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

    // Manage idle timeouts. This is co-located with inspect() for thread safety.
    watchdog.manage();
    wifiManager.loopMaintenance(anyActive);
    vTaskDelay(pdMS_TO_TICKS(g_SystemPower ? 1 : 20));
  }
}
