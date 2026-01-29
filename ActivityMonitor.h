/**
 * @file ActivityMonitor.h
 * @brief Logic for idle timeout detection
 */
#ifndef ACTIVITY_MONITOR_H
#define ACTIVITY_MONITOR_H

#include <Arduino.h>
#include <LocoNetStreamESP32.h>

class ActivityMonitor {
  private:
    uint32_t lastActivity;
    const uint32_t timeoutMs;

  public:
    ActivityMonitor(uint32_t timeout);
    void reset();  // Replaces init()
    void inspect(const lnMsg *p);
    bool shouldTrigger();
};

#endif
