#include "NetworkInterface.h"
#include "AsyncDebug.h"
#include "ActivityMonitor.h"

// These objects are defined in the main .ino file.
// Declaring them here allows this task to access them.
// This centralizes all activity-related logic to this task for thread-safety.
extern ActivityMonitor watchdog;

/*
 * @brief Queues a raw data buffer to be sent to network clients.
 * 
 * This overload is for mutable data. It calculates the LocoNet checksum
 * before queueing the packet.
 * @param data The raw byte buffer to process and send.
 * @param len The length of the data buffer.
 */
static inline void sendSpoofedPacket(uint8_t* data, size_t len) {
    if (len < 2) return; // Basic safety for checksum
    uint8_t chk = 0;
    for (size_t i = 0; i < len - 1; i++) chk ^= data[i];
    data[len - 1] = ~chk;

    lnMsg msg;
    memcpy(msg.data, data, len);
    xQueueSend(lnToNetQueue, &msg, 0);
}

/**
 * @brief Queues a raw data buffer to be sent to network clients.
 * 
 * This overload is for immutable (const) data that has a pre-calculated
 * checksum. It skips the checksum calculation.
 * @param data The raw byte buffer to send.
 * @param len The length of the data buffer.
 */
static inline void sendSpoofedPacket(const uint8_t* data, size_t len) {
    lnMsg msg;
    memcpy(msg.data, data, len);
    xQueueSend(lnToNetQueue, &msg, 0);
}

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
              // Log the valid incoming command immediately for clear diagnostics.
              LOG_DEBUG("RX[%d]: %s\n", i, inbound.asChars);
              client.write(rsp_ok, 9);
              if (g_SystemPower) {
                xQueueSend(netToLnQueue, &tx, 0);
              } else {
                // Power Off: Echo and Spoof Response

                xQueueSend(lnToNetQueue, &tx, 0);
                if (tx.data[0] == 0xBB) { // Slot Read Request
                  switch(tx.data[1]) {
                    case 0x79: { // Special case: IB Status Query
                      const uint8_t raw[] = { 0xB4, 0x3B, 0x00, 0x70 };
                      sendSpoofedPacket(raw, sizeof(raw));
                      LOG_DEBUG("RX[%d]: %s (Spoofed B4 - IB Status)\n", i, inbound.asChars);
                      break;
                    }
                    case 0x00: { // Special case: IB Identity Query
                      const uint8_t raw[] = { 0xE7, 0x0E, 0x00, 0x00, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x49, 0x42, 0x19 };
                      sendSpoofedPacket(raw, sizeof(raw));
                      LOG_DEBUG("RX[%d]: %s (Spoofed E7 - IB Identity)\n", i, inbound.asChars);
                      break;
                    }
                    default: { // General case: Any other slot query
                      // Respond with an empty/inactive slot message to prevent client request-loops.
                      uint8_t slot = tx.data[1];
                      uint8_t raw[] = { 0xE7, 0x0E, slot, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                      sendSpoofedPacket(raw, sizeof(raw));
                      LOG_DEBUG("RX[%d]: %s (Spoofed E7 - Empty Slot %d)\n", i, inbound.asChars, slot);
                      break;
                    }
                  }
                } else if (tx.data[0] == 0x83) { // Power ON command
                  uint8_t raw[] = { 0x82, 0x00 };
                  sendSpoofedPacket(raw, sizeof(raw));
                  LOG_DEBUG("RX[%d]: %s (Spoofed 82 - Power Off)\n", i, inbound.asChars);
                } else if (tx.data[0] != 0x82) { // Not a power off command
                  // Spoof Long Ack (0xB4) for other commands
                  uint8_t raw[] = { 0xB4, (uint8_t)(tx.data[0] & 0x7F), 0x00, 0x00 };
                  sendSpoofedPacket(raw, sizeof(raw));
                  LOG_DEBUG("RX[%d]: %s (Spoofed B4 - Generic Ack)\n", i, inbound.asChars);
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
