#include "PowerLine.h"
#include "Common.h" // Provides: g_SystemPower, g_TrackPower, lnToNetQueue
#include "AsyncDebug.h"
#include "LocoNetPackets.h"   

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
  g_SystemPower = (debouncer.read() == HIGH);
  LOG_DEBUG("PowerLine: Pin %d. System: %s\n", pin, g_SystemPower ? "ON" : "OFF");
  xTaskCreate(PowerLine::task, "PowerMon", 2048, this, tskIDLE_PRIORITY, &taskHandle);
}

void PowerLine::task(void* param) {
  PowerLine* self = (PowerLine*)param;
  for (;;) {
    self->debouncer.update();
    g_SystemPower = (self->debouncer.read() == HIGH);
    if (self->debouncer.fell()) {
        LOG_DEBUG("System Power: OFF\n");
        g_TrackPower = false;
        if (lnToNetQueue != NULL) {
            xQueueSend(lnToNetQueue, (void *)&PACKET_GP_OFF, 0);
        }
    }
    
    if (self->debouncer.rose()) {
        LOG_DEBUG("System Power: ON\n");
    }

    // OPTIMIZATION: Relaxed polling for physical switch
    // 50ms (20Hz) is sufficient for human interaction
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
