#ifndef POWERLINE_H
#define POWERLINE_H

#include <Arduino.h>
#include <Bounce2.h>

class PowerLine {
  private:
    Bounce debouncer;
    int pin;

  public:
    PowerLine();
    void begin(int p);
    bool isOn();
};

#endif
