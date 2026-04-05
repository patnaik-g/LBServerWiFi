#include "PowerLine.h"
#include "Config.h"
#include "Common.h" 
#include "AsyncDebug.h"
#include "LocoNetPackets.h"   
#include "driver/uart.h"

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
  xTaskCreate(PowerLine::task, "PowerMon", 2048, this, tskIDLE_PRIORITY, &taskHandle);
}

void PowerLine::task(void* param) {
  PowerLine* self = (PowerLine*)param;
  for (;;) {
    self->debouncer.update();
    
    if (self->debouncer.fell()) {
        LOG_DEBUG("PWR: OFF\n");
        g_TrackPower = false;
        
        uart_driver_delete((uart_port_t)1);
        pinMode(PIN_LOCONET_TX, OUTPUT);
        digitalWrite(PIN_LOCONET_TX, LOW);
        pinMode(PIN_LOCONET_RX, INPUT_PULLUP);

        if (lnToNetQueue != NULL) {
            xQueueSend(lnToNetQueue, (void *)&PACKET_GP_OFF, 0);
        }
    }
    
    if (self->debouncer.rose()) {
        LOG_DEBUG("PWR: ON\n");
        pinMode(PIN_LOCONET_TX, OUTPUT);
        digitalWrite(PIN_LOCONET_TX, LOW);
        if (self->_lnStream) self->_lnStream->start();
    }

    g_SystemPower = (self->debouncer.read() == HIGH);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
