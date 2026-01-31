/**
 * @file WiFiManager
 * @brief Transport Layer Orchestrator
 */
#include "WiFiManager.h"
#include "AsyncDebug.h" 

WiFiEventCallback WiFiManager::eventCallback = nullptr;

WiFiManager::WiFiManager(const char* hostname, uint16_t port, WiFiEventCallback callback) 
   : hostname(hostname), server(port), port(port) {
  eventCallback = callback;
  for(int i=0; i<MAX_CLIENTS; i++) {
    clientPool[i] = WiFiClient();
    clientActive[i] = false;
  }
}

void WiFiManager::begin() {
  prefs.begin("wifi", false);
  LOG_DEBUG("\n[BOOT] Waiting 2s. Send 'w' to WIPE settings...\n");
  unsigned long t = millis();
  while (millis() - t < 2000) {
      if (Serial.available()) {
          char c = Serial.read();
          if (c == 'w' || c == 'W') {
              LOG_DEBUG("\n!!! WIPE COMMAND RECEIVED !!!\n");
              prefs.clear();
              prefs.end();
              for(int i=0; i<10; i++) {
                  digitalWrite(PIN_STATUS_LED, HIGH);
                  delay(50);
                  digitalWrite(PIN_STATUS_LED, LOW);  delay(50);
              }
              ESP.restart();
          }
      }
      delay(10);
  }

  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  String host = prefs.getString("hostname", this->hostname);
  this->hostname = strdup(host.c_str());
  
  if (ssid.length() > 0 && connectToWiFi(ssid, password)) {
    prefs.end();
  } else {
    startAPMode();
  }
}

bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
  WiFi.setHostname(hostname);
  WiFi.begin(ssid.c_str(), password.c_str());
  WiFi.setTxPower(WIFI_POWER_11dBm);
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    LOG_DEBUG("WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    
    ArduinoOTA.begin();
    TelnetStream.begin();
    // Standard mDNS Setup
    if (MDNS.begin(hostname)) {
        MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTO, port);
        MDNS.addService("arduino", "tcp", 3232); // Helps the IDE find the port
    } else {
        LOG_DEBUG("Error setting up MDNS responder!\n");
    }
    
    server.begin();
    return true;
  }
  return false;
}

void WiFiManager::checkNewConnections() {
  if (server.hasClient()) {
    bool assigned = false;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!clientActive[i]) {
        clientPool[i] = server.available();
        if (clientPool[i]) {
           clientPool[i].setNoDelay(true);
           clientPool[i].print("VERSION " BRIDGE_VERSION "\r\n");
           clientActive[i] = true;
           assigned = true;
        }
        break;
      }
    }
  }
}

void WiFiManager::loopMaintenance(bool anyActive) {
  if (!anyActive) {
    digitalWrite(PIN_STATUS_LED, HIGH);
    ArduinoOTA.handle();
  } else {
    if (millis() - lastBlink > 500) {
      ledState = !ledState;
      digitalWrite(PIN_STATUS_LED, ledState);
      lastBlink = millis();
    }
  }
}

void WiFiManager::forEachActiveClient(std::function<void(WiFiClient&, int index)> callback) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!clientActive[i]) continue;
    if (!clientPool[i].connected()) {
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
    webServer.send(200, "text/html", "<h2>WiFi Setup</h2>");
  });
  webServer.begin();
  while (true) { webServer.handleClient(); }
}

WiFiServer& WiFiManager::getServer() { return server; }
