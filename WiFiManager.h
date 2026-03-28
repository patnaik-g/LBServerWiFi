/*
 * @file WiFiManager
 * @brief Transport Layer Orchestrator
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "Config.h" 
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>
#include <ESPmDNS.h>
#include <functional>

typedef void (*WiFiEventCallback)(WiFiEvent_t event);
class WiFiManager {
public:
  WiFiManager(const char* hostname, uint16_t port = DEFAULT_PORT, WiFiEventCallback callback = nullptr);
  void begin();
  WiFiServer& getServer();
  void checkNewConnections();
  void loopMaintenance(bool anyActive);
  void broadcast(const char* data, size_t len);
  void forEachActiveClient(std::function<void(WiFiClient&, int index)> callback);

private:
  const char* hostname;
  uint16_t port;
  WiFiServer server;
  WebServer webServer;
  Preferences prefs;
  unsigned long lastBlink = 0;
  bool ledState = true;
  
  static WiFiEventCallback eventCallback;
  
  WiFiClient clientPool[MAX_CLIENTS]; 
  bool clientActive[MAX_CLIENTS];
  bool connectToWiFi(const String& ssid, const String& password);
  void startAPMode();
  void logHeapStatus();
};

#endif
