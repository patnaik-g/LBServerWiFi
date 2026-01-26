/*
 * LBServerWiFi v2.0.0 - NetworkInterface.cpp
 * PROTOCOL: LBServer (LocoNet-over-TCP)
 * DESCRIPTION: High-performance, multi-client WiFi bridge implementing the LBServer 
 * protocol for seamless integration with JMRI and other LocoNet controllers.
 */

#include "NetworkInterface.h"
#include "AsyncDebug.h"

// Limit to 3 to ensure stability
// Exceeding this risks LwIP socket exhaustion on the ESP32.
#define MAX_CLIENTS 3

WiFiManager wifiManager("LBServer");
WiFiClient clientPool[MAX_CLIENTS];
bool clientActive[MAX_CLIENTS]; // Explicit state tracking to prevent "zombie" connections

/**
 * Protocol Buffer Union
 * Forces 4-byte memory alignment. This allows us to cast the 
 * first 4 bytes of the buffer to a uint32_t for single-cycle 
 * comparison (checking for "SEND" signature) instead of byte-by-byte.
 */
union ProtocolBuffer {
    char asChars[256];
    uint32_t asUint32[64]; 
};

/**
 * Optimized ASCII Hex to Byte Helper
 * Uses raw arithmetic rather than library functions for speed.
 * Converts two ASCII characters (e.g., 'F', 'A') into one byte (0xFA).
 */
static inline uint8_t fastHexToByte(char high, char low) {
    auto toN = [](char c) -> uint8_t {
        if (c >= 'A') return c - 'A' + 10;
        return c - '0';
    };
    return (toN(high) << 4) | (toN(low) & 0x0F);
}

/**
 * LocoNet Packet Length Helper
 * Determines the length of a LocoNet message based on its OpCode.
 */
static inline uint8_t getPacketLen(const lnMsg *p) {
    uint8_t opc = p->data[0];
    switch (opc & 0x60) {
        case 0x00: return 2;      // 2-byte packet
        case 0x20: return 4;      // 4-byte packet
        case 0x40: return 6;      // 6-byte packet
        case 0x60: return p->data[1]; // Variable length (length is in byte 2)
        default: return 0;
    }
}

void communicationTask(void *pvParameters) {
    wifiManager.begin();
    pinMode(PIN_STATUS_LED, OUTPUT);
    
    ProtocolBuffer inbound; 
    char out[128]; 
    char rsp_ok[10]; 
    
    // Pre-calculate fixed strings to save cycles in the loop
    out[0] = 'R'; out[1] = 'E'; out[2] = 'C'; out[3] = 'E';
    out[4] = 'I'; out[5] = 'V'; out[6] = 'E';
    
    rsp_ok[0] = 'S'; rsp_ok[1] = 'E'; rsp_ok[2] = 'N'; rsp_ok[3] = 'T';
    rsp_ok[4] = ' '; rsp_ok[5] = 'O'; rsp_ok[6] = 'K'; 
    rsp_ok[7] = '\r'; rsp_ok[8] = '\n'; rsp_ok[9] = '\0';
    
    // "SEND" signature in little-endian hex (ASCII: S=0x53, E=0x45, N=0x4E, D=0x44)
    const uint32_t SIG_SEND = 0x444E4553; 

    // MINIMAL EDIT: Variables for inverted LED logic
    unsigned long lastBlink = 0;
    bool ledState = true;

    // Init Pool and Flags
    for(int i=0; i<MAX_CLIENTS; i++) {
        clientPool[i] = WiFiClient();
        clientActive[i] = false;
    }

    for (;;) {
        // --- 1. NEW CONNECTION HANDLING ---
        WiFiServer& server = wifiManager.getServer();
        if (server.hasClient()) {
            bool assigned = false;
            // Scan for a slot explicitly marked as 'Inactive'
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clientActive[i]) {
                    clientPool[i] = server.available(); 
                    if (clientPool[i]) { 
                        // Configure for low latency (disable Nagle's algorithm)
                        clientPool[i].setNoDelay(true);
                        clientPool[i].setTimeout(10);
                        
                        // UPDATED: Use shared version from Header
                        clientPool[i].print("VERSION " BRIDGE_VERSION "\r\n");
                        
                        LOG_DEBUG(">>> Client [%d] Connected from %s\n", i, clientPool[i].remoteIP().toString().c_str());
                        clientActive[i] = true; // Mark slot as Occupied
                        assigned = true;
                    }
                    break;
                }
            }
            // Reject if no slots are free to protect memory
            if (!assigned) {
                WiFiClient reject = server.available();
                LOG_DEBUG(">>> Rejected %s: Pool Full\n", reject.remoteIP().toString().c_str());
                reject.stop();
            }
        }

        // --- 2. INBOUND PROCESSING (Network -> LocoNet Queue) ---
        bool anyActive = false;
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            // Only process slots we marked as Active
            if (!clientActive[i]) continue; 

            // 2a. Read Data (Drain Buffer)
            // Critical: We must read ALL data before checking for disconnects
            while (clientPool[i].available() > 0) {
                size_t len = clientPool[i].readBytesUntil('\n', inbound.asChars, 255);
                
                // Fast check: Does buffer start with "SEND"?
                if (len >= 4 && *(uint32_t*)inbound.asChars == SIG_SEND) {
                    inbound.asChars[len] = '\0';
                    LOG_DEBUG("RX[%d]: %s\n", i, inbound.asChars);
                    
                    // Parse Hex string into byte array
                    lnMsg tx; uint8_t txIdx = 0;
                    char *p = inbound.asChars + 4; // Skip "SEND"
                    while (p < (inbound.asChars + len) && txIdx < sizeof(tx.data)) {
                        if (*p <= 32) { p++; continue; } // Skip whitespace
                        if (*p && *(p + 1)) {
                            tx.data[txIdx++] = fastHexToByte(*p, *(p + 1));
                            p += 2;
                        } else p++;
                    }

                    // Send to Hardware Queue
                    if (txIdx > 0 && xQueueSend(netToLnQueue, &tx, 0) == pdPASS) {
                        clientPool[i].write(rsp_ok, 9); // Ack ONLY the sender
                        LOG_DEBUG("Ack: %s", rsp_ok);
                    }
                }
            }
            
            // 2b. Check Disconnect
            // Must happen AFTER reading; if buffer was non-empty, connected() would return true
            // even if the socket was closed. Now that buffer is empty, we get the real state.
            if (!clientPool[i].connected()) {
                LOG_DEBUG(">>> Client [%d] Disconnected\n", i);
                clientPool[i].stop(); 
                clientActive[i] = false; // Mark slot as Free
                continue;
            }
            
            anyActive = true;
        }
        
        // MINIMAL EDIT: Inverted LED Logic (Steady Idle, Blink Connected)
        if (!anyActive) {
            digitalWrite(PIN_STATUS_LED, HIGH);
            ledState = true;
        } else {
            if (millis() - lastBlink > 500) {
                ledState = !ledState;
                digitalWrite(PIN_STATUS_LED, ledState);
                lastBlink = millis();
            }
        }

        // --- 3. OUTBOUND PROCESSING (LocoNet Queue -> Broadcast) ---
        // This handles both real traffic from the rails AND the "Echo" loopback
        lnMsg rx;
        if (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
            uint8_t pktLen = getPacketLen(&rx);
            static const char hex[] = "0123456789ABCDEF";
            
            // Format: "RECEIVE <HEX>..."
            int pos = 7; 
            for (uint8_t i = 0; i < pktLen; i++) {
                out[pos++] = ' ';
                out[pos++] = hex[(rx.data[i] >> 4) & 0x0F];
                out[pos++] = hex[rx.data[i] & 0x0F];
            }
            out[pos++] = '\r'; out[pos++] = '\n';
            
            // Broadcast to ALL active clients
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clientActive[i] && clientPool[i].connected()) {
                    clientPool[i].write(out, pos);
                }
            }
            
            out[pos] = '\0';
            LOG_DEBUG("BCAST: %s", out);
        }

        vTaskDelay(1); // Yield to other tasks
    }
}
