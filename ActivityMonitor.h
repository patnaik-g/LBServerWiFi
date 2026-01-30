/*
 * @file ActivityMonitor.h
 * @brief Logic for idle timeout detection and safety enforcement
 */

#ifndef ACTIVITY_MONITOR_H
#define ACTIVITY_MONITOR_H

#include <Arduino.h>
#include <LocoNetStreamESP32.h>
#include "NetworkInterface.h"

#ifdef SYSTEM_POWER_CONTROL
class KasaPlug; // Forward declaration
#endif

class ActivityMonitor {
  private:
    // Core State
    uint32_t lastActivity;
    bool _wasSystemOff;
    const uint32_t trackTimeoutMs;
    
    // Core Dependencies
    LocoNetStreamESP32* _lnStream;
    QueueHandle_t _lnToNetQueue;

    // Feature: System Power Control
#ifdef SYSTEM_POWER_CONTROL
    const uint32_t systemTimeoutMs;
    KasaPlug* _plug;
#endif

    // Helpers (Encapsulated Logic)
    bool shouldTriggerTrackOff();
    bool checkSystemTimeout();     // Always declared (implementation changes)
    void performSystemShutdown();  // Always declared (implementation changes)

  public:
    // Constructor & Setup
#ifdef SYSTEM_POWER_CONTROL
    ActivityMonitor(uint32_t trackTimeout, uint32_t systemTimeout);
    void begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue, KasaPlug* plug);
#else
    ActivityMonitor(uint32_t trackTimeout);
    void begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue);
#endif

    // Common Runtime
    bool isSystemOff(); 
    void reset();
    void inspect(const lnMsg *p);
    void manage();
};

#endif
