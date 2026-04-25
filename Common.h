/*
 * @file Common.h
 * @brief System-wide Global State Declarations
 */

#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include "Config.h"

// Global System State
#ifdef ENABLE_POWER_MONITOR
  // If power monitoring is enabled, declare g_SystemPower as a global variable.
  // Its state will be managed by the PowerLine task.
  extern volatile bool g_SystemPower;
#else
  // If power monitoring is disabled, define g_SystemPower as a compile-time constant 'true'.
  // This makes the system behave as if power is always on.
  #define g_SystemPower true
#endif
extern volatile bool g_TrackPower;
extern uint32_t lastTrafficMilli;

// Global Message Queues
extern QueueHandle_t lnToNetQueue;
extern QueueHandle_t netToLnQueue;
#endif
