#include "PowerLine.h"
#include "Config.h"
#include "Common.h" 
#include "AsyncDebug.h"
#include "LocoNetPackets.h"   

#ifdef ENABLE_POWER_MONITOR

PowerLine::PowerLine() {
  debouncer = Bounce();
  taskHandle = NULL;
  _lnStream = NULL;
}

void PowerLine::begin(int p, LocoNetStreamESP32* lnStream) {
  pin = p;
  _lnStream = lnStream;
  pinMode(pin, INPUT);
  debouncer.attach(pin, INPUT);
  debouncer.interval(50);
  debouncer.update();
  g_SystemPower = (debouncer.read() == HIGH);
  LOG_DEBUG("PowerLine: Pin %d. System: %s\n", pin, g_SystemPower ? "ON" : "OFF");

  if (g_SystemPower && _lnStream) {
      vTaskDelay(pdMS_TO_TICKS(200)); // Stabilization delay
      LOG_DEBUG("PWR: Requesting Status (Slot 0)...\n");
      _lnStream->send((lnMsg*)&PACKET_REQ_SLOT0);
  }

  xTaskCreate(PowerLine::task, "PowerMon", 2048, this, tskIDLE_PRIORITY, &taskHandle);
}

void PowerLine::task(void* param) {
  PowerLine* self = (PowerLine*)param;
  for (;;) {
    self->debouncer.update();
    if (self->debouncer.fell()) {
        LOG_DEBUG("PWR: OFF\n");
        g_TrackPower = false;
        if (lnToNetQueue != NULL) {
            xQueueSend(lnToNetQueue, (void *)&PACKET_GP_OFF, 0);
        }
    }
    
    if (self->debouncer.rose()) {
        LOG_DEBUG("PWR: ON\n");
    }

    g_SystemPower = (self->debouncer.read() == HIGH);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

#endif
