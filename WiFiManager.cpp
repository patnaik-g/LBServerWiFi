#include "WiFiManager.h"

// Uncomment the following line to clear stored WiFi credentials and hostname
//#define CLEARPREFS

WiFiEventCallback WiFiManager::eventCallback = nullptr;

WiFiManager::WiFiManager(const char* hostname, uint16_t port, WiFiEventCallback callback)
  : hostname(hostname), server(port), port(port) {
  eventCallback = callback;
}

void WiFiManager::begin() {
  prefs.begin("wifi", false);

#ifdef CLEARPREFS
  prefs.clear();
  LOG_DEBUG("NVM CLEARED: WiFi credentials and hostname reset.\n");
#endif

  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  
  // This will now pull "LBServer" from the constructor if NVM is empty
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
  // Print connection details immediately
    LOG_DEBUG("\n-----------------------------------\n");
    LOG_DEBUG("WiFi Connected!\n");
    LOG_DEBUG("IP Address: %s\n", WiFi.localIP().toString().c_str()); 
    LOG_DEBUG("mDNS Hostname: %s.local\n", hostname);
    LOG_DEBUG("-----------------------------------\n");
    //ArduinoOTA.setPassword("update");
    ArduinoOTA.begin();
    TelnetStream.begin();
    while (!MDNS.begin(hostname)) { delay(500); }
    MDNS.addService(MDNS_SERVICE_NAME, MDNS_SERVICE_PROTO, port);
    server.begin();
    return true;
  }
  return false;
}

void WiFiManager::startAPMode() {
  LOG_DEBUG("Starting AP Mode...\n");
  WiFi.softAP(hostname);  // Use hostname as AP SSID

  LOG_DEBUG("AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());

  webServer.on("/", [this]() {
    webServer.send(200, "text/html",
                   "<!DOCTYPE html>"
                   "<html>"
                   "<head>"
                   "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                   "<style>"
                   "body { font-family: Arial, sans-serif; text-align: center; padding: 20px; font-size: 24px; }"
                   "div { display: inline-block; text-align: center; background: #f3f3f3; padding: 20px; border-radius: 10px; }"
                   "h2 { color: #333; }"
                   "p { font-size: 20px; color: #555; }"
                   "input { font-size: 20px; padding: 5px; width: 80%; margin: 5px 0; }"
                   "input[type='submit'] { background: #007bff; color: white; border: none; padding: 10px; border-radius: 5px; cursor: pointer; }"
                   "input[type='submit']:hover { background: #0056b3; }"
                   "</style>"
                   "</head>"
                   "<body>"
                   "<div>"
                   "<h2>WiFi Setup</h2>"
                   "<form action='/save' method='POST'>"
                   "SSID: <input type='text' name='ssid' required><br>"
                   "Password: <input type='password' name='password' required><br>"
                   "Hostname: <input type='text' name='hostname' value='"
                     + String(hostname) + "' required><br>"
                                          "<input type='submit' value='Save'>"
                                          "</form>"
                                          "</div>"
                                          "</body>"
                                          "</html>");
  });

  webServer.on("/save", [this]() {
    String ssid = webServer.arg("ssid");
    String password = webServer.arg("password");
    String hostname = webServer.arg("hostname");

    if (ssid.length() > 0 && password.length() > 0 && hostname.length() > 0) {
      prefs.putString("ssid", ssid);
      prefs.putString("password", password);
      prefs.putString("hostname", hostname);
      LOG_DEBUG("Credentials saved. Restarting...\n");
      webServer.send(200, "text/html",
                     "<!DOCTYPE html>"
                     "<html>"
                     "<head>"
                     "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                     "<style>"
                     "body { font-family: Arial, sans-serif; text-align: center; padding: 20px; font-size: 24px; }"
                     "div { display: inline-block; text-align: center; background: #f3f3f3; padding: 20px; border-radius: 10px; }"
                     "h2 { color: #333; }"
                     "p { font-size: 20px; color: #555; }"
                     "</style>"
                     "</head>"
                     "<body>"
                     "<div>"
                     "<h2>Settings Saved</h2>"
                     "<p>Restarting...</p>"
                     "</div>"
                     "</body>"
                     "</html>");

      delay(1000);
      ESP.restart();
    } else {
      webServer.send(400, "text/html", "Invalid input. Please enter both SSID and password.");
    }
  });

  webServer.begin();
  LOG_DEBUG("AP Mode Ready. Connect and go to: http://192.168.4.1\n");

  while (true) {
    webServer.handleClient();
  }
}

WiFiServer& WiFiManager::getServer() { return server; }
