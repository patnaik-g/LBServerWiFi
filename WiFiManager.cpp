/**
 * @file WiFiManager
 * @brief Transport Layer Orchestrator
 */

#include "WiFiManager.h"
#include "NetworkInterface.h" 

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

  // --- BOOT MENU ---
  // A clean, software-only way to reset settings without buttons.
  LOG_DEBUG("\n[BOOT] Waiting 2s. Send 'w' to WIPE settings...\n");
  
  unsigned long t = millis();
  while (millis() - t < 2000) {
      if (Serial.available()) {
          char c = Serial.read();
          if (c == 'w' || c == 'W') {
              LOG_DEBUG("\n!!! WIPE COMMAND RECEIVED !!!\n");
              LOG_DEBUG("Clearing preferences...\n");
              prefs.clear();
              prefs.end();
              
              // Visual Confirmation (Rapid Blink)
              for(int i=0; i<10; i++) {
                  digitalWrite(PIN_STATUS_LED, HIGH); delay(50);
                  digitalWrite(PIN_STATUS_LED, LOW);  delay(50);
              }
              LOG_DEBUG("Restarting...\n");
              ESP.restart();
          }
      }
      delay(10);
  }
  LOG_DEBUG("[BOOT] Continuing...\n");

  // --- NORMAL STARTUP ---
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

  LOG_DEBUG("Connecting to WiFi: %s\n", ssid.c_str());
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    LOG_DEBUG(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    LOG_DEBUG("\n-----------------------------------\n");
    LOG_DEBUG("WiFi Connected!\n");
    LOG_DEBUG("IP Address: %s\n", WiFi.localIP().toString().c_str()); 
    LOG_DEBUG("mDNS Hostname: %s.local\n", hostname);
    LOG_DEBUG("-----------------------------------\n");
    
    ArduinoOTA.begin();
    TelnetStream.begin();

    while (!MDNS.begin(hostname)) { delay(500); }
    MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTO, port);
    
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
          clientPool[i].setTimeout(10);
          clientPool[i].print("VERSION " BRIDGE_VERSION "\r\n");
          LOG_DEBUG(">>> Client [%d] Connected from %s\n", i, clientPool[i].remoteIP().toString().c_str());
          clientActive[i] = true; 
          assigned = true;
        }
        break;
      }
    }

    if (!assigned) {
      WiFiClient reject = server.available();
      LOG_DEBUG(">>> Rejected %s: Pool Full\n", reject.remoteIP().toString().c_str());
      reject.stop();
    }
  }
}

void WiFiManager::loopMaintenance(bool anyActive) {
  if (!anyActive) {
    // Steady LED and OTA available when idle
    digitalWrite(PIN_STATUS_LED, HIGH);
    ledState = true;
    ArduinoOTA.handle();   
  } else {
    // Blink LED every 500ms when network is active
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
      LOG_DEBUG(">>> Client [%d] Disconnected\n", i);
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
  LOG_DEBUG("Starting AP Mode...\n");
  WiFi.softAP(hostname);
  
  webServer.on("/", [this]() {
    webServer.send(200, "text/html", "<h2>WiFi Setup</h2><form action='/save' method='POST'>SSID: <input type='text' name='ssid'><br>Password: <input type='password' name='password'><br>Hostname: <input type='text' name='hostname' value='" + String(hostname) + "'><br><input type='submit' value='Save'></form>");
  });
  
  webServer.on("/save", [this]() {
    String ssid = webServer.arg("ssid");
    String password = webServer.arg("password");
    String hostname = webServer.arg("hostname");
    
    if (ssid.length() > 0 && password.length() > 0 && hostname.length() > 0) {
      prefs.putString("ssid", ssid);
      prefs.putString("password", password);
      prefs.putString("hostname", hostname);
      webServer.send(200, "text/html", "<h2>Settings Saved</h2><p>Restarting...</p>");
      delay(1000);
      ESP.restart();
    }
  });
  
  webServer.begin();
  while (true) {
    webServer.handleClient();
  }
}

WiFiServer& WiFiManager::getServer() { return server; }
