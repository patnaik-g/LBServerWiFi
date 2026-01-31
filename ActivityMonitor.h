/*
 * @file ActivityMonitor.h
 * @brief Logic for idle timeout detection and safety enforcement
 */

#ifndef ACTIVITY_MONITOR_H
#define ACTIVITY_MONITOR_H

#include "Config.h"
#include "Common.h" // Provides: g_SystemPower, g_TrackPower
#include <Arduino.h>
#include <LocoNetStreamESP32.h>

#ifdef SYSTEM_POWER_CONTROL
class KasaPlug;
#endif

class ActivityMonitor {
  private:
    uint32_t lastActivity;
    bool _wasSystemOff;
    const uint32_t trackTimeoutMs;
    
    LocoNetStreamESP32* _lnStream;
    QueueHandle_t _lnToNetQueue;
#ifdef SYSTEM_POWER_CONTROL
    const uint32_t systemTimeoutMs;
    KasaPlug* _plug;
#endif

    bool shouldTriggerTrackOff();
    bool checkSystemTimeout();
    void performSystemShutdown();
  public:
#ifdef SYSTEM_POWER_CONTROL
    ActivityMonitor(uint32_t trackTimeout, uint32_t systemTimeout);
    void begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue, KasaPlug* plug);
#else
    ActivityMonitor(uint32_t trackTimeout);
    void begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue);
#endif

    bool isSystemOff(); 
    void reset();
    void inspect(const lnMsg *p);
    void manage();
};

#endif
