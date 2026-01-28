/**
 * @file LocoNetPackets.h
 * @brief Predefined LocoNet Messages
 */
#ifndef LOCONET_PACKETS_H
#define LOCONET_PACKETS_H

#include <LocoNetStreamESP32.h>
#include "ln_opc.h" 

// Global Power OFF
// Opcode: OPC_GPOFF (0x82)
// Checksum: 0xFF XOR 0x82 == ~0x82 (0x7D)
static const lnMsg PACKET_GP_OFF = { {OPC_GPOFF, (uint8_t)(~OPC_GPOFF)} };

#endif
