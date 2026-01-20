#ifndef LOCONET_INTERFACE_H
#define LOCONET_INTERFACE_H

#include <Arduino.h>
#include <LocoNetStreamESP32.h>
#include <LocoNetStream.h>

#define LOCONET_PIN_RX 22
#define LOCONET_PIN_TX 23
#define PIN_STATUS_LED 2 

extern LocoNetBus bus;
extern LocoNetDispatcher parser;
extern LocoNetStreamESP32 lnStream;

uint8_t hexToByte(char high, char low);
uint8_t getPacketLen(const lnMsg *p);
bool processLbServerCommand(char* cmd, QueueHandle_t target);
void processTelnetCommand(char* cmd, QueueHandle_t target);

#endif