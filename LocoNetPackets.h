#ifndef LOCONET_PACKETS_H
#define LOCONET_PACKETS_H

#include <LocoNetStreamESP32.h>

// OPC_GPOFF (0x82) - Global Power OFF
static const lnMsg PACKET_GP_OFF = { .data = { 0x82, 0x7D } };
// OPC_GPON (0x83) - Global Power ON
static const lnMsg PACKET_GP_ON  = { .data = { 0x83, 0x7C } };
// OPC_IDLE (0x85) - Force Idle State
static const lnMsg PACKET_IDLE   = { .data = { 0x85, 0x7A } };
// OPC_RQ_SL_DATA (0xBB) - Request Slot 0 Data
static const lnMsg PACKET_REQ_SLOT0 = { .data = { 0xBB, 0x00, 0x00, 0x44 } };
#endif
