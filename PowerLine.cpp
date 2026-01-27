#include "PowerLine.h"

PowerLine::PowerLine() {
  debouncer = Bounce();
}

void PowerLine::begin(int p) {
  pin = p;
  debouncer.attach(pin, INPUT);
  debouncer.interval(50); // 50ms debounce
}

bool PowerLine::isOn() {
  debouncer.update();
  return (debouncer.read() == HIGH);
}
