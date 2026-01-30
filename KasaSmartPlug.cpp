#include "KasaSmartPlug.h"

const char *CMD_INFO = "{\"system\":{\"get_sysinfo\":null}}";
const char *CMD_ON   = "{\"system\":{\"set_relay_state\":{\"state\":1}}}";
const char *CMD_OFF  = "{\"system\":{\"set_relay_state\":{\"state\":0}}}";

KasaPlug::KasaPlug(const char *ipAddress) {
    strcpy(ip, ipAddress);
    dest_addr.sin_addr.s_addr = inet_addr(ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(9999);
    sock = -1;
}

KasaPlug* KasaPlug::Find(const char* targetAlias, int timeoutMs, int maxRetries) {
    int attempt = 0;
    while(attempt++ < maxRetries) {
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) return NULL;

        int bcast = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));

        char buf[128];
        struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(9999) };
        addr.sin_addr.s_addr = inet_addr("255.255.255.255");
        
        sendto(sock, buf, Encrypt(CMD_INFO, strlen(CMD_INFO), 0, buf), 0, (struct sockaddr *)&addr, sizeof(addr));

        struct timeval tv = { .tv_sec = 0, .tv_usec = (long)timeoutMs * 1000 };
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        while (select(sock + 1, &rfds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(sock, &rfds)) {
                char rBuf[1024], rIp[32] = {0};
                struct sockaddr_storage rAddr;
                socklen_t rLen = sizeof(rAddr);
                
                int n = recvfrom(sock, rBuf, sizeof(rBuf)-1, 0, (struct sockaddr *)&rAddr, &rLen);
                if (n > 0) {
                    n = Decrypt(rBuf, n, rBuf, 0);
                    rBuf[n] = 0;
                    
                    StaticJsonDocument<1024> sDoc;
                    if (n > 50 && !deserializeJson(sDoc, rBuf)) {
                        if (strcmp(sDoc["system"]["get_sysinfo"]["alias"], targetAlias) == 0) {
                            if (rAddr.ss_family == PF_INET) {
                                inet_ntoa_r(((struct sockaddr_in *)&rAddr)->sin_addr, rIp, sizeof(rIp)-1);
                            }
                            close(sock);
                            return new KasaPlug(rIp);
                        }
                    }
                }
            }
            tv.tv_sec = 0; tv.tv_usec = 50000;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
        }
        close(sock);
        delay(2000); 
    }
    return NULL;
}

bool KasaPlug::OpenSock() {
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) return false;
    struct timeval tv = {1, 0}; 
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        CloseSock();
        return false;
    }
    return true;
}

void KasaPlug::CloseSock() {
    if (sock != -1) {
        shutdown(sock, 0);
        close(sock);
        sock = -1;
    }
}

int KasaPlug::SendQuery(const char *cmd, char *buf, int len, long timeout) {
    int rLen = 0;
    if(OpenSock()) {
        char sBuf[128];
        send(sock, sBuf, Encrypt(cmd, strlen(cmd), 1, sBuf), 0);
        rLen = recv(sock, buf, len, 0);
        if (rLen > 0) rLen = Decrypt(buf, rLen, buf, 4);
        CloseSock();
    }
    return rLen;
}

int KasaPlug::SyncState() {
    char buf[1024];
    int len = SendQuery(CMD_INFO, buf, 1024, 500000);
    if (len > 50 && !deserializeJson(doc, buf, len)) {
        state = doc["system"]["get_sysinfo"]["relay_state"];
        return state;
    }
    return -1;
}

bool KasaPlug::SetRelay(uint8_t target) {
    char buf[64];
    return SendQuery(target ? CMD_ON : CMD_OFF, buf, 64, 500000) > 0;
}

bool KasaPlug::SetRelayVerified(uint8_t target, int retries) {
    for(int i=0; i<retries; i++) {
        SetRelay(target);
        delay(500); 
        if (SyncState() == target) return true;
        delay(1000);
    }
    return false;
}

uint16_t KasaPlug::Encrypt(const char *d, int len, uint8_t add, char *out) {
    uint8_t k = KASA_KEY;
    int idx = 0;
    if (add) { out[idx++] = 0; out[idx++] = 0; out[idx++] = len >> 8; out[idx++] = len & 0xFF; }
    for (int i=0; i<len; i++) { out[idx++] = d[i] ^ k; k = out[idx-1]; }
    return idx;
}

uint16_t KasaPlug::Decrypt(char *d, int len, char *out, int start) {
    uint8_t k = KASA_KEY;
    int r = 0;
    for (int i=start; i<len; i++) { uint8_t dc = d[i] ^ k; k = d[i]; out[r++] = dc; }
    return r;
}
