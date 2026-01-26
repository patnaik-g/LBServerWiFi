/**
 * @file WiFiManager
 * @brief Transport Layer Orchestrator
 * * Manages the TCP server lifecycle, WiFi connectivity, and system maintenance 
 * (OTA/mDNS). Implements an opaque client pool using a functional iterator 
 * (forEachActiveClient) to maintain protocol agnosticism.
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>
#include <ESPmDNS.h>
#include "AsyncDebug.h"
#include <functional>

#define MDNS_SERVICE_NAME "lbserver"
#define MDNS_SERVICE_PROTO "tcp"
#define MAX_CLIENTS 3

typedef void (*WiFiEventCallback)(WiFiEvent_t event);

class WiFiManager {
public:
  WiFiManager(const char* hostname, uint16_t port = 1234, WiFiEventCallback callback = nullptr);
  void begin();
  WiFiServer& getServer();
  void checkNewConnections();
  void loopMaintenance(bool anyActive);
  void broadcast(const char* data, size_t len); // NEW: Outbound abstraction
  void forEachActiveClient(std::function<void(WiFiClient&, int index)> callback);

private:
  const char* hostname;
  uint16_t port;
  WiFiServer server;
  WebServer webServer;
  Preferences prefs;
  unsigned long lastBlink = 0; // Track LED timing
  bool ledState = true;
  static WiFiEventCallback eventCallback;
  WiFiClient clientPool[MAX_CLIENTS];
  bool clientActive[MAX_CLIENTS];
  bool connectToWiFi(const String& ssid, const String& password);
  void startAPMode();
};

#endif
