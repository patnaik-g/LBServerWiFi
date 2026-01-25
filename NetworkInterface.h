#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "WiFiManager.h"
#include "LocoNetStreamESP32.h"

#define BRIDGE_VERSION "ESP32 LocoNet Bridge v2.0.0"

// --- QUEUE HANDLES ---
// These allow the main loop and the network task to talk to each other
extern QueueHandle_t lnToNetQueue;
extern QueueHandle_t netToLnQueue;

// --- TASKS ---
void communicationTask(void *pvParameters);

// --- HARDWARE PIN DEFINITIONS ---
// (Keep these if you had them here, otherwise ignore)
#define PIN_STATUS_LED 2

#endif
