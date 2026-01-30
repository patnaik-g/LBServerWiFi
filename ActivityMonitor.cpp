#include "ActivityMonitor.h"
#include "LocoNetPackets.h" 
#include "NetworkInterface.h"
#include "AsyncDebug.h"     
#include "KasaSmartPlug.h"  

ActivityMonitor::ActivityMonitor(uint32_t trackTimeout, uint32_t systemTimeout) 
    : lastActivity(0), _wasSystemOff(false), 
      trackTimeoutMs(trackTimeout), systemTimeoutMs(systemTimeout),
      _lnStream(nullptr), _lnToNetQueue(nullptr), _plug(nullptr) {}

void ActivityMonitor::begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue, KasaPlug* plug) {
    _lnStream = lnStream;
    _lnToNetQueue = lnToNetQueue;
    _plug = plug;
}

// Check Hardware Gate AND Update Internal State
bool ActivityMonitor::isSystemOff() {
    if (!g_SystemPower) {
        _wasSystemOff = true;
        return true;
    }
    return false;
}

void ActivityMonitor::reset() {
    lastActivity = millis();
}

void ActivityMonitor::inspect(const lnMsg *p) {
    uint8_t opc = p->data[0];
    
    if (opc == OPC_GPON)  { g_TrackPower = true;  lastActivity = millis(); }
    if (opc == OPC_GPOFF) { g_TrackPower = false; }

    if (g_TrackPower) {
        bool active = false;
        if (opc == OPC_SW_REQ) active = true;
        if (opc == OPC_LOCO_SPD && p->data[2] > 0) active = true;
        
        if (active) lastActivity = millis();
    }
}

bool ActivityMonitor::shouldTriggerTrackOff() {
    if (!g_SystemPower) return false;
    if (g_TrackPower && (millis() - lastActivity > trackTimeoutMs)) {
        g_TrackPower = false; 
        return true;
    }
    return false;
}

bool ActivityMonitor::shouldTriggerSystemOff() {
    if (!g_SystemPower) return false;
    if (millis() - lastActivity > systemTimeoutMs) {
        return true;
    }
    return false;
}

void ActivityMonitor::manage() {
    // 1. Handle Wake-Up Logic (Moved from Main Loop)
    if (_wasSystemOff) {
        LOG_DEBUG("System Power Restored. Resetting Idle Timer.\n");
        reset();
        _wasSystemOff = false;
    }

    // 2. TIER 1: Track Power (15 Min)
    if (shouldTriggerTrackOff()) {
        LOG_DEBUG("Idle Timeout (15m). Track Power OFF.\n");
        if (_lnStream) _lnStream->send((lnMsg*)&PACKET_GP_OFF);
        if (_lnToNetQueue) xQueueSend(_lnToNetQueue, (void*)&PACKET_GP_OFF, 0);
    }

    // 3. TIER 2: System Power (30 Min)
    if (shouldTriggerSystemOff()) {
        LOG_DEBUG("Idle Timeout (30m). System Power OFF.\n");
        if (_plug) {
             _plug->SetRelayVerified(0);
        } else {
             LOG_DEBUG("Error: No Kasa Plug configured.\n");
             reset(); 
        }
    }
}
