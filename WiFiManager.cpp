/**
 * @file WiFiManager
 * @brief Transport Layer Orchestrator
 */
#include "WiFiManager.h"
#include "AsyncDebug.h" 

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

void WiFiManager::begin() {
  prefs.begin("wifi", false);
  Serial.println("\n[BOOT] Waiting 2s. Send 'w' to WIPE settings...");
  
  unsigned long t = millis();
  while (millis() - t < 2000) {
      if (Serial.available()) {
          char c = Serial.read();
          if (c == 'w' || c == 'W') {
              Serial.println("\n!!! WIPE COMMAND RECEIVED !!!");
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
  
  static char finalHostname[32];
  strncpy(finalHostname, host.c_str(), 31);
  this->hostname = finalHostname;
  
  if (ssid.length() > 0 && connectToWiFi(ssid, password)) {
    prefs.end();
    // Default: Solid ON (0 active clients initially)
    digitalWrite(PIN_STATUS_LED, HIGH);
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
    Serial.printf("WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    vTaskDelay(pdMS_TO_TICKS(1000));
    MDNS.end();
    delay(100);
    
    if (MDNS.begin(hostname)) {
        MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTO, port);
        #ifdef ENABLE_TELNET_LOGGING
        MDNS.addService("telnet", "tcp", 23);
        #endif
        
        Serial.printf("mDNS responder started: %s.local\n", hostname);
    } else {
        Serial.println("Error setting up MDNS responder!");
    }
    
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.begin();
    
    #ifdef ENABLE_TELNET_LOGGING
    TelnetStream.begin();
    #endif
    
    server.begin();
    return true;
  }
  return false;
}

void WiFiManager::checkNewConnections() {
  if (server.hasClient()) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!clientActive[i]) {
        clientPool[i] = server.available();
        if (clientPool[i]) {
           clientPool[i].setNoDelay(true);
           LOG_DEBUG("Client %d connected from %s\n", i, clientPool[i].remoteIP().toString().c_str());
           clientPool[i].print("VERSION " BRIDGE_VERSION "\r\n");
           clientActive[i] = true;
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
    // Optimization: Single time sample
    uint32_t now = millis();
    
    // 10s Heartbeat
    if (now - lastTrafficMilli > 10000) {
      digitalWrite(PIN_STATUS_LED, HIGH);
      lastTrafficMilli = now;
    }
    
    // Snap Off (10ms pulse)
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
  Serial.printf("AP Mode Started. Connect to %s to configure WiFi.\n", hostname);

  webServer.on("/", [this]() {
    String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head>";
    html += "<body><h2>" + String(hostname) + " WiFi Setup</h2>";
    html += "<form action='/save' method='POST'>";
    html += "Hostname: <br><input type='text' name='hostname' value='" + String(hostname) + "'><br>";
    html += "SSID: <br><input type='text' name='ssid'><br>";
    html += "Password: <br><input type='password' name='pass'><br><br>";
    html += "<input type='submit' value='Save and Reboot'></form></body></html>";
    webServer.send(200, "text/html", html);
  });

  webServer.on("/save", [this]() {
    String newSsid = webServer.arg("ssid");
    String newPass = webServer.arg("pass");
    String newHost = webServer.arg("hostname");
    
    // Header for mobile scaling
    String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head><body>";
    
    if (newSsid.length() > 0) {
      prefs.putString("ssid", newSsid);
      prefs.putString("password", newPass);
      if (newHost.length() > 0) prefs.putString("hostname", newHost);
      
      html += "<h2>Settings saved. Rebooting...</h2></body></html>";
  
      webServer.send(200, "text/html", html);
      
      delay(2000);
      ESP.restart();
    } else {
      html += "<h2>Error: SSID cannot be empty.</h2><a href='/'>Back</a></body></html>";
      webServer.send(200, "text/html", html);
    }
  });

  webServer.begin();
  while (true) { webServer.handleClient(); }
}

WiFiServer& WiFiManager::getServer() { return server; }
