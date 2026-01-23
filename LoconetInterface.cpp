#include "LoconetInterface.h"

// THE DEFINITIONS - These must exist exactly once in the project
LocoNetBus bus;
LocoNetDispatcher parser(&bus);
LocoNetStreamESP32 lnStream(1, LOCONET_PIN_RX, LOCONET_PIN_TX, false, true, &bus);

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
