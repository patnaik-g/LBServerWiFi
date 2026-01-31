/*
 * @file LocoNetPackets.h
 * @brief Shared LocoNet Protocol Definitions
 *
 * PURPOSE:
 * This file acts as a "Common Block" for static packet definitions.
 * It allows unrelated modules (ActivityMonitor, PowerLine) to use the 
 * same constant packet structures without being coupled to the NetworkInterface.
 */

#ifndef LOCONET_PACKETS_H
#define LOCONET_PACKETS_H

#include <LocoNetStreamESP32.h>

// OPC_GPOFF (0x83) - Global Power OFF
// Used by: ActivityMonitor (Timeout), PowerLine (Button Press)
static const lnMsg PACKET_GP_OFF = { .data = { 0x83, 0x7C } };

// OPC_GPON (0x82) - Global Power ON
// Used by: PowerLine (Button Press)
static const lnMsg PACKET_GP_ON  = { .data = { 0x82, 0x7D } };

// OPC_IDLE (0x85) - Force Idle State
static const lnMsg PACKET_IDLE   = { .data = { 0x85, 0x7A } };

#endif
