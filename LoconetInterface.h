#ifndef LOCONET_INTERFACE_H
#define LOCONET_INTERFACE_H

#include <Arduino.h>
#include <LocoNetStreamESP32.h>
#include <LocoNetStream.h>

/**
 * Hardware Pin Mapping
 */
#define LOCONET_PIN_RX 22
#define LOCONET_PIN_TX 23

/**
 * Global Hardware Objects (Declarations)
 * 'extern' tells the compiler these exist in another translation unit.
 */
extern LocoNetBus bus;
extern LocoNetDispatcher parser;
extern LocoNetStreamESP32 lnStream;

/**
 * Utility: getPacketLen
 * Decodes the LocoNet OpCode to determine binary message length.
 */
uint8_t getPacketLen(const lnMsg *p);

#endif
