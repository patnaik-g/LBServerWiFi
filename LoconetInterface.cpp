#include "LoconetInterface.h"
#include "debug.h"

LocoNetBus bus;
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1, LOCONET_PIN_RX, LOCONET_PIN_TX, false, true, &bus);

uint8_t hexToByte(char high, char low) {
    auto toN = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    return (toN(high) << 4) | toN(low);
}

uint8_t getPacketLen(const lnMsg *p) {
    uint8_t opc = p->data[0];
    switch (opc & 0x60) {
        case 0x00: return 2;
        case 0x20: return 4;
        case 0x40: return 6;
        case 0x60: return p->data[1];
        default: return 0;
    }
}

bool processLbServerCommand(char* cmd, QueueHandle_t target) {
    if (strncmp(cmd, "SEND", 4) == 0) {
        lnMsg tx; uint8_t len = 0; int i = 4;
        while (cmd[i] != '\0' && len < sizeof(tx.data)) {
            if (isspace(cmd[i])) { i++; continue; }
            if (isxdigit(cmd[i]) && isxdigit(cmd[i+1])) {
                tx.data[len++] = hexToByte(cmd[i], cmd[i+1]);
                i += 2;
            } else i++;
        }
        return (len > 0) ? (xQueueSend(target, &tx, 0) == pdPASS) : false;
    }
    return false;
}

void processTelnetCommand(char* cmd, QueueHandle_t target) {
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        LOG_DEBUG("\n==================================\n   LBServer Wireless Console\n==================================\nLocoNet: SEND <hex bytes>\nSystem:  status, reboot, help\n----------------------------------\n\n");
        return;
    }
    if (strncmp(cmd, "SEND", 4) == 0) processLbServerCommand(cmd, target);
}