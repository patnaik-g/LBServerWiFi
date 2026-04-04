#ifndef POWERLINE_H
#define POWERLINE_H

#include <Arduino.h>
#include <Bounce2.h>
#include "Config.h"

#ifdef SYSTEM_POWER_CONTROL
#include <LocoNetStreamESP32.h>
#endif

class PowerLine {
  private:
    Bounce debouncer;
    int pin;
    TaskHandle_t taskHandle;
    
#ifdef SYSTEM_POWER_CONTROL
    LocoNetStreamESP32* _lnStream;
#endif
    
    static void task(void* param);

  public:
    PowerLine();
#ifdef SYSTEM_POWER_CONTROL
    void begin(int p, LocoNetStreamESP32* lnStream);
#else
    void begin(int p);
#endif
};

#endif
