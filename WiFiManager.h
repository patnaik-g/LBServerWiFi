#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <TelnetStream.h>
#include "AsyncDebug.h"

// Network & mDNS Configuration
#define DEFAULT_TCP_PORT 1234
#define MDNS_SERVICE_NAME "loconetovertcpserver"
#define MDNS_SERVICE_PROTO "tcp"

typedef void (*WiFiEventCallback)();

class WiFiManager {
public:
    WiFiManager(const char* hostname, uint16_t port = DEFAULT_TCP_PORT, WiFiEventCallback callback = nullptr);
    void begin();
    WiFiServer& getServer();

private:
    static void WiFiEvent(WiFiEvent_t event);
    void startAPMode();
    bool connectToWiFi(const String &ssid, const String &password);

    const char* hostname;
    uint16_t port;
    WiFiServer server;
    Preferences prefs;
    WebServer webServer = WebServer(80);
    static WiFiEventCallback eventCallback;
};

#endif
