#include "NetworkInterface.h"
#include "LoconetInterface.h"
#include "debug.h"

WiFiManager wifiManager("LBServer");
WiFiClient &netClient = wifiManager.getClient();

void communicationTask(void *pvParameters) {
    wifiManager.begin();
    pinMode(PIN_STATUS_LED, OUTPUT);
    
    char buf[256], telnetBuf[128];
    int idx = 0, telnetIdx = 0;
    bool lastNetConnected = false;
    unsigned long lastHeartbeat = 0, lastBlink = 0;
    bool ledState = false;

    for (;;) {
        bool currentNetConnected = netClient.connected();
        if (!currentNetConnected) {
            netClient = wifiManager.getServer().available();
            currentNetConnected = netClient.connected();
            if (currentNetConnected && !lastNetConnected) {
                netClient.println("VERSION ESP32 LocoNet Bridge v1.5");
                LOG_DEBUG(">>> JMRI Client Connected\n");
                lastNetConnected = true;
                digitalWrite(PIN_STATUS_LED, HIGH);
            }
        }

        if (millis() - lastHeartbeat > 20000) {
            processTelnetCommand((char*)"help", netToLnQueue);
            LOG_DEBUG("STATUS | JMRI: %s | Uptime: %lu min\n", currentNetConnected ? "ON" : "OFF", millis() / 60000);
            lastHeartbeat = millis();
        }

        if (!currentNetConnected) {
            ArduinoOTA.handle();
            if (millis() - lastBlink > 500) {
                ledState = !ledState;
                digitalWrite(PIN_STATUS_LED, ledState);
                lastBlink = millis();
            }
        } else {
            while (netClient.available()) {
                char c = netClient.read();
                if (c == '\n' || c == '\r') {
                    if (idx > 0) {
                        buf[idx] = '\0';
                        if (processLbServerCommand(buf, netToLnQueue)) netClient.println("SENT OK");
                        idx = 0;
                    }
                } else if (idx < sizeof(buf) - 1) buf[idx++] = c;
            }
        }

        while (TelnetStream.available()) {
            char c = TelnetStream.read();
            if (c == '\n' || c == '\r') {
                if (telnetIdx > 0) {
                    telnetBuf[telnetIdx] = '\0';
                    if (strcmp(telnetBuf, "reboot") == 0) ESP.restart();
                    else if (strcmp(telnetBuf, "status") == 0) lastHeartbeat = 0; // Trigger heartbeat
                    else processTelnetCommand(telnetBuf, netToLnQueue);
                    telnetIdx = 0;
                } else processTelnetCommand((char*)"help", netToLnQueue);
            } else if (telnetIdx < sizeof(telnetBuf) - 1) telnetBuf[telnetIdx++] = c;
        }

        lnMsg rx;
        if (xQueueReceive(lnToNetQueue, &rx, 0) == pdPASS) {
            uint8_t len = getPacketLen(&rx);
            char out[128]; int pos = sprintf(out, "RECEIVE");
            for (uint8_t i = 0; i < len; i++) pos += sprintf(out + pos, " %02X", rx.data[i]);
            if (currentNetConnected) netClient.println(out);
            LOG_DEBUG("DEBUG: %s\n", out);
        }
        vTaskDelay(1);
    }
}