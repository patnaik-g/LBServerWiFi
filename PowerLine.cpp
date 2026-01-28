#include "PowerLine.h"
#include "NetworkInterface.h" 
#include "AsyncDebug.h"
#include "LocoNetPackets.h"   

// Global Definition
volatile bool g_PowerState = true; 

PowerLine::PowerLine() {
  debouncer = Bounce();
  taskHandle = NULL;
}

void PowerLine::begin(int p) {
  pin = p;
  pinMode(pin, INPUT);
  debouncer.attach(pin, INPUT);
  debouncer.interval(50); 

  // Initial state check
  debouncer.update();
  g_PowerState = (debouncer.read() == HIGH);
  
  LOG_DEBUG("PowerLine: Monitor started on Pin %d. Initial State: %s\n", pin, g_PowerState ? "ON" : "OFF");

  xTaskCreate(
    PowerLine::task, 
    "PowerMon", 
    2048, 
    this, 
    tskIDLE_PRIORITY, 
    &taskHandle
  );
}

void PowerLine::task(void* param) {
  PowerLine* self = (PowerLine*)param;
  bool lastState = g_PowerState;

  for (;;) {
    self->debouncer.update();
    bool currentState = (self->debouncer.read() == HIGH);

    // Direct write to global
    g_PowerState = currentState;

    if (currentState != lastState) {
      LOG_DEBUG("Power Status: %s\n", currentState ? "ON" : "OFF");
      lastState = currentState;

      if (lnToNetQueue != NULL) {
          xQueueSend(lnToNetQueue, (void *)&PACKET_GP_OFF, 0);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
