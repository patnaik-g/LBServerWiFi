#ifndef POWERLINE_H
#define POWERLINE_H

#include <Arduino.h>
#include <Bounce2.h>

class PowerLine {
  private:
    Bounce debouncer;
    int pin;
    TaskHandle_t taskHandle;
    
    // FreeRTOS Task Wrapper
    static void task(void* param);

  public:
    PowerLine();
    void begin(int p);
};

#endif
