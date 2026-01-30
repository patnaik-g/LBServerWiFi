/**
 * @file ActivityMonitor.h
 * @brief Logic for idle timeout detection and safety enforcement
 */
#ifndef ACTIVITY_MONITOR_H
#define ACTIVITY_MONITOR_H

#include <Arduino.h>
#include <LocoNetStreamESP32.h>

// Forward declaration
class KasaPlug; 

class ActivityMonitor {
  private:
    // State
    uint32_t lastActivity;
    bool _wasSystemOff; // Tracks previous power state

    // Configuration
    const uint32_t trackTimeoutMs;
    const uint32_t systemTimeoutMs;

    // Dependencies
    LocoNetStreamESP32* _lnStream;
    QueueHandle_t _lnToNetQueue;
    KasaPlug* _plug;

    // Internal Helpers
    bool shouldTriggerTrackOff();
    bool shouldTriggerSystemOff();

  public:
    ActivityMonitor(uint32_t trackTimeout, uint32_t systemTimeout);
    
    // Setup
    void begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue, KasaPlug* plug);
    
    // Runtime
    bool isSystemOff(); // New: Handles Hardware Gate & State Tracking
    void reset();
    void inspect(const lnMsg *p);
    void manage();
};

#endif
