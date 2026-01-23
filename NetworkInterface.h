#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

/* Standard and Custom Library Dependencies */
#include <WiFi.h>          // ESP32 WiFi stack for TCP/IP communication
#include <ArduinoOTA.h>    // Support for Over-The-Air firmware updates
#include "WiFiManager.h"   // Custom manager for WiFi provisioning and persistence

/* UI / Status Mapping - Moved here from LoconetInterface */
#define PIN_STATUS_LED 2

/* * Global Queue Handles 
 * These queues facilitate asynchronous communication between the 
 * Network Task and the LocoNet Hardware Task.
 */
extern QueueHandle_t lnToNetQueue; // Outbound traffic: LocoNet hardware -> WiFi client
extern QueueHandle_t netToLnQueue; // Inbound traffic: WiFi client -> LocoNet hardware

/**
 * @brief Entry point for the asynchronous network communication task.
 * * Manages the TCP socket, handshaking, and bidirectional message 
 * translation between the LBServer protocol and LocoNet packets.
 * * @param pvParameters FreeRTOS task parameters.
 */
void communicationTask(void *pvParameters);

#endif
