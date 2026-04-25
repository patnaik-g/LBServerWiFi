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

#ifdef ENABLE_KASA_CONTROL
class KasaPlug;
#endif

class ActivityMonitor {
  private:
    // --- Core Members ---
    uint32_t lastActivity;
    const uint32_t trackTimeoutMs;
    LocoNetStreamESP32* _lnStream;
    QueueHandle_t _lnToNetQueue;

    // --- Conditional Members ---
#ifdef ENABLE_POWER_MONITOR
    bool _wasSystemOff;
#endif
#ifdef ENABLE_KASA_CONTROL
    const uint32_t systemTimeoutMs;
    KasaPlug* _plug;
#endif

    bool shouldTriggerTrackOff(uint32_t now);
    bool checkSystemTimeout(uint32_t now);
    void performSystemShutdown();
public:
#ifdef ENABLE_KASA_CONTROL
    ActivityMonitor(uint32_t trackTimeout, uint32_t systemTimeout);
    void begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue, KasaPlug* plug);
#else
    ActivityMonitor(uint32_t trackTimeout);
    void begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue);
#endif

    void reset();
    void inspect(const lnMsg *p);
    void manage();
};

#endif
