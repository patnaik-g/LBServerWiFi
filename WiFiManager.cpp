/**
 * @file WiFiManager
 * @brief Transport Layer Orchestrator
 */
#include "WiFiManager.h"
#include "AsyncDebug.h" 
#include "Common.h"
#include "LocoNetPackets.h"
#include "lwip/sockets.h"

#if defined(ENABLE_HEAP_MONITOR)
#include <esp_heap_caps.h>
#endif

extern uint32_t lastTrafficMilli;
WiFiEventCallback WiFiManager::eventCallback = nullptr;

WiFiManager::WiFiManager(const char* hostname, uint16_t port, WiFiEventCallback callback) 
   : hostname(hostname), server(port), port(port) {
  eventCallback = callback;
  for(int i=0; i<MAX_CLIENTS; i++) {
    clientPool[i] = WiFiClient();
    clientActive[i] = false;
  }
}

void WiFiManager::logHeapStatus() {
#if defined(ENABLE_HEAP_MONITOR) && (defined(ENABLE_SERIAL_LOGGING) || defined(ENABLE_TELNET_LOGGING))
    uint32_t f = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    uint32_t l = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    LOG_DEBUG("SYS: Free=%u, MaxBlock=%u, Frag=%.1f%%\n", f, l, 100.0 - ((float)l/f * 100.0));
#endif
}

void WiFiManager::begin() {
  prefs.begin("wifi", false);
#ifdef ENABLE_SERIAL_LOGGING
  logHeapStatus();
  Serial.println("\n[BOOT] Waiting 2s. Send 'w' to WIPE settings...");
  unsigned long t = millis();
  while (millis() - t < 2000) {
      if (Serial.available()) {
          char c = Serial.read();
          if (c == 'w' || c == 'W') {
              prefs.clear();
              prefs.end();
              ESP.restart();
          }
      }
      delay(10);
  }
  #endif

  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  String host = prefs.getString("hostname", this->hostname);
  static char finalHostname[32];
  strncpy(finalHostname, host.c_str(), 31);
  this->hostname = finalHostname;

  if (ssid.length() > 0 && connectToWiFi(ssid, password)) {
    prefs.end();
    digitalWrite(PIN_STATUS_LED, HIGH);
    logHeapStatus();
  } else {
    startAPMode();
  }
}

bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
  WiFi.setHostname(hostname);
  WiFi.setSleep(false);
  WiFi.begin(ssid.c_str(), password.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    MDNS.end();
    delay(100);
    if (MDNS.begin(hostname)) {
        MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTO, port);
#ifdef ENABLE_TELNET_LOGGING
        MDNS.addService("telnet", "tcp", 23);
#endif
    }
    
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.begin();
    #ifdef ENABLE_TELNET_LOGGING
    TelnetStream.begin();
    #endif
    server.begin();
    server.setNoDelay(true);
    return true;
  }
  return false;
}

void WiFiManager::checkNewConnections() {
  if (server.hasClient()) {
    bool slotFound = false;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!clientActive[i]) {
        clientPool[i] = server.available();
        if (clientPool[i]) {
           int fd = clientPool[i].fd();
           int enable = 1;
           int idle = TCP_KEEP_IDLE;
           int interval = TCP_KEEP_INTVL;
           int count = TCP_KEEP_CNT;
           setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
           setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
           setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
           setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
           clientPool[i].setNoDelay(true);
           LOG_DEBUG("Client %d connected\n", i);
           
           clientPool[i].print("VERSION " BRIDGE_VERSION "\r\n");
           
           // Synchronize Client State via Queue
           if (lnToNetQueue != NULL) {
               xQueueSend(lnToNetQueue, (void*)(g_TrackPower ? &PACKET_GP_ON : &PACKET_GP_OFF), 0);
           }
           
           clientActive[i] = true;
        }
        slotFound = true;
        break;
      }
    }
    if (!slotFound) {
        WiFiClient rejected = server.available();
        rejected.stop();
        LOG_DEBUG("Connection rejected: MAX_CLIENTS reached\n");
    }
  }
}

void WiFiManager::loopMaintenance(bool anyActive) {
  static uint32_t lastHeapLog = 0;
  uint32_t now = millis();
  if (now - lastHeapLog > 60000) {
      logHeapStatus();
      lastHeapLog = now;
  }

  if (!anyActive) {
    digitalWrite(PIN_STATUS_LED, HIGH);
    ArduinoOTA.handle();
  } else {
     if (digitalRead(PIN_STATUS_LED) == HIGH && (now - lastTrafficMilli > 10)) {
      digitalWrite(PIN_STATUS_LED, LOW);
    }
  }
}

void WiFiManager::forEachActiveClient(std::function<void(WiFiClient&, int index)> callback) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!clientActive[i]) continue;
    if (!clientPool[i].connected()) {
      LOG_DEBUG("Client %d disconnected\n", i);
      clientPool[i].stop();
      clientActive[i] = false;
      continue;
    }
    callback(clientPool[i], i);
  }
}

void WiFiManager::broadcast(const char* data, size_t len) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientActive[i] && clientPool[i].connected()) {
      clientPool[i].write(data, len);
    }
  }
}

void WiFiManager::startAPMode() {
  WiFi.softAP(hostname);
  webServer.on("/", [this]() {
    String html = "<html><body><h2>WiFi Setup</h2><form action='/save' method='POST'>";
    html += "Hostname: <input type='text' name='hostname' value='" + String(hostname) + "'><br>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "Pass: <input type='password' name='pass'><br>";
    html += "<input type='submit' value='Save'></form></body></html>";
    webServer.send(200, "text/html", html);
  });
  webServer.on("/save", [this]() {
    prefs.putString("ssid", webServer.arg("ssid"));
    prefs.putString("password", webServer.arg("pass"));
    prefs.putString("hostname", webServer.arg("hostname"));
    webServer.send(200, "text/html", "Rebooting...");
    delay(2000);
    ESP.restart();
  });
  webServer.begin();
  while (true) { webServer.handleClient(); }
}

WiFiServer& WiFiManager::getServer() { return server;
}
