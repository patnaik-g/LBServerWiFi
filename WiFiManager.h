#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>
#include <ESPmDNS.h>
#include "AsyncDebug.h"

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
  void loopMaintenance(bool anyActive); // New maintenance method

  WiFiClient clientPool[MAX_CLIENTS];
  bool clientActive[MAX_CLIENTS];

private:
  const char* hostname;
  uint16_t port;
  WiFiServer server;
  WebServer webServer;
  Preferences prefs;
  unsigned long lastBlink = 0; // Track LED timing
  bool ledState = true;
  static WiFiEventCallback eventCallback;

  bool connectToWiFi(const String& ssid, const String& password);
  void startAPMode();
};

#endif
