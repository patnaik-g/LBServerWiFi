#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

#include <WiFi.h>
#include <ArduinoOTA.h>
#include "WiFiManager.h"

extern QueueHandle_t lnToNetQueue;
extern QueueHandle_t netToLnQueue;

void communicationTask(void *pvParameters);

#endif