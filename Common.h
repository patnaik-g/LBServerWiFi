/*
 * @file Common.h
 * @brief System-wide Global State Declarations
 */

#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>

// Global System State
extern volatile bool g_SystemPower;
extern volatile bool g_TrackPower;

// Global Message Queues
extern QueueHandle_t lnToNetQueue;
extern QueueHandle_t netToLnQueue;

#endif
