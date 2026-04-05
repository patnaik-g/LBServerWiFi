#ifndef POWERLINE_H
#define POWERLINE_H

#include <Arduino.h>
#include <Bounce2.h>
#include "Config.h"
#include <LocoNetStreamESP32.h>

class PowerLine {
  private:
    Bounce debouncer;
    int pin;
    TaskHandle_t taskHandle;
    LocoNetStreamESP32* _lnStream;
    
    static void task(void* param);

  public:
    PowerLine();
    void begin(int p, LocoNetStreamESP32* lnStream);
};

#endif
