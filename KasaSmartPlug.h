#ifndef KASA_SMART_PLUG_H
#define KASA_SMART_PLUG_H

#include <WiFi.h>
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include <ArduinoJson.h>

#define KASA_KEY 171

class KasaPlug {
private:
    struct sockaddr_in dest_addr;
    int sock;
    StaticJsonDocument<512> doc;
    bool _isShutdownLatched;
    
    bool OpenSock();
    void CloseSock();
    int SendQuery(const char *cmd, char *buf, int len, long timeout);
    static uint16_t Encrypt(const char *data, int len, uint8_t addLen, char *out);
    static uint16_t Decrypt(char *data, int len, char *out, int start);

public:
    char ip[32];
    int state;

    KasaPlug(const char *ipAddress);

    bool SetRelay(uint8_t targetState);
    bool SetRelayVerified(uint8_t targetState, int maxRetries = 5);
    void resetShutdownLatch();
    bool attemptShutdown(int retries = 5);
    int SyncState();
    
    // Static Factory
    static KasaPlug* Find(const char* targetAlias, int timeoutMs = 1500, int maxRetries = 10);
};
#endif
