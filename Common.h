/*
 * @file Common.h
 * @brief System-wide Global State Declarations
 */

#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include "Config.h"

// Global System State
#ifdef SYSTEM_POWER_CONTROL
extern volatile bool g_SystemPower;
#endif
extern volatile bool g_TrackPower;
extern uint32_t lastTrafficMilli;

// Global Message Queues
extern QueueHandle_t lnToNetQueue;
extern QueueHandle_t netToLnQueue;
#endif
