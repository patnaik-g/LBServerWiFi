#include "ActivityMonitor.h"
#include "LocoNetPackets.h" 
#include "AsyncDebug.h"      

// --- 1. FEATURE IMPLEMENTATION (Enabled/Disabled Variants) ---

#ifdef SYSTEM_POWER_CONTROL
#include "KasaSmartPlug.h" 

// [ENABLED] Constructor & Setup
ActivityMonitor::ActivityMonitor(uint32_t trackTimeout, uint32_t systemTimeout) 
    : lastActivity(0), _wasSystemOff(false), 
      trackTimeoutMs(trackTimeout), systemTimeoutMs(systemTimeout),
      _lnStream(nullptr), _lnToNetQueue(nullptr), _plug(nullptr) {}

void ActivityMonitor::begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue, KasaPlug* plug) {
    _lnStream = lnStream;
    _lnToNetQueue = lnToNetQueue;
    _plug = plug;
}

// [ENABLED] Helper Logic
bool ActivityMonitor::checkSystemTimeout(uint32_t now) {
    if (!g_SystemPower) return false;
    return (now - lastActivity > systemTimeoutMs);
}

void ActivityMonitor::performSystemShutdown() {
    LOG_DEBUG("Idle Timeout (%d min). System Power OFF.\n", systemTimeoutMs / 60000);
    if (_plug) { 
        _plug->SetRelayVerified(0);
    } else { 
        LOG_DEBUG("Error: No Kasa Plug configured.\n");
        reset();
    }
}

#else

// [DISABLED] Constructor & Setup
ActivityMonitor::ActivityMonitor(uint32_t trackTimeout) 
    : lastActivity(0), _wasSystemOff(false), 
      trackTimeoutMs(trackTimeout),
      _lnStream(nullptr), _lnToNetQueue(nullptr) {}

void ActivityMonitor::begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue) {
    _lnStream = lnStream;
    _lnToNetQueue = lnToNetQueue;
}

// [DISABLED] Stubs (Optimized away by compiler)
bool ActivityMonitor::checkSystemTimeout(uint32_t now) { return false;
}
void ActivityMonitor::performSystemShutdown() { /* No-Op */ }

#endif


// --- 2. COMMON LOGIC ---

bool ActivityMonitor::isSystemOff() {
#ifdef SYSTEM_POWER_CONTROL
    if (!g_SystemPower) {
        _wasSystemOff = true;
        return true;
    }
#endif
    return false;
}

void ActivityMonitor::reset() {
    lastActivity = millis();
}

void ActivityMonitor::inspect(const lnMsg *p) {
    uint8_t opc = p->data[0];
    uint32_t now = millis();
    if (opc == OPC_GPON)  { 
        g_TrackPower = true;  
        lastActivity = now;
    }
    if (opc == OPC_GPOFF) { 
        g_TrackPower = false;
    }

    if (g_TrackPower) {
        bool active = false;
        if (opc == OPC_SW_REQ) active = true;
        if (opc == OPC_LOCO_SPD && p->data[2] > 0) active = true;
        if (active) lastActivity = now;
    }
}

bool ActivityMonitor::shouldTriggerTrackOff(uint32_t now) {
#ifdef SYSTEM_POWER_CONTROL
    if (!g_SystemPower) return false;
#endif
    if (g_TrackPower && (now - lastActivity > trackTimeoutMs)) {
        g_TrackPower = false;
        return true;
    }
    return false;
}

void ActivityMonitor::manage() {
    uint32_t now = millis();
    // 1. Handle Wake-Up Logic
    if (_wasSystemOff) {
        LOG_DEBUG("System Power Restored. Resetting Idle Timer.\n");
        lastActivity = now;
        _wasSystemOff = false;
    }

    // 2. TIER 1: Track Power
    if (shouldTriggerTrackOff(now)) {
        LOG_DEBUG("Idle Timeout (%d min). Track Power OFF.\n", trackTimeoutMs / 60000);
        if (_lnStream) _lnStream->send((lnMsg*)&PACKET_GP_OFF);
        if (_lnToNetQueue) xQueueSend(_lnToNetQueue, (void*)&PACKET_GP_OFF, 0);
    }

    // 3. TIER 2: System Power
    if (checkSystemTimeout(now)) {
        performSystemShutdown();
    }
}
