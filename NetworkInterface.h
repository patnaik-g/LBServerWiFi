/**
 * @file NetworkInterface
 * @brief LocoNet-over-TCP Protocol Handler
 */
#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include <Arduino.h>
#include <WiFi.h>
#include "WiFiManager.h" 
#include "LocoNetStreamESP32.h"

#define BRIDGE_VERSION "2.3.0"
#define PIN_STATUS_LED 2

// --- GLOBAL STATE ---
extern volatile bool g_SystemPower; // Defined in PowerLine.cpp
extern volatile bool g_TrackPower;  // Defined in PowerLine.cpp

extern QueueHandle_t lnToNetQueue;
extern QueueHandle_t netToLnQueue;
extern WiFiManager wifiManager;

union ProtocolBuffer {
    char asChars[256];
    uint32_t asUint32[64];
};

static inline uint8_t fastHexToByte(char high, char low) {
    auto toN = [](char c) -> uint8_t {
        if (c >= 'A') return c - 'A' + 10;
        if (c >= 'a') return c - 'a' + 10;
        return c - '0';
    };
    return (toN(high) << 4) | (toN(low) & 0x0F);
}

static inline uint8_t getPacketLen(const lnMsg *p) {
    uint8_t opc = p->data[0];
    switch (opc & 0x60) {
        case 0x00: return 2;
        case 0x20: return 4;
        case 0x40: return 6;
        case 0x60: return p->data[1];
        default: return 0;
    }
}

void communicationTask(void *pvParameters);
#endif
