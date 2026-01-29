#include "ActivityMonitor.h"
#include "LocoNetPackets.h" 
#include "NetworkInterface.h" // Access g_TrackPower

ActivityMonitor::ActivityMonitor(uint32_t timeout) 
    : lastActivity(0), timeoutMs(timeout) {}

void ActivityMonitor::reset() {
    lastActivity = millis();
}

void ActivityMonitor::inspect(const lnMsg *p) {
    uint8_t opc = p->data[0];
    
    // UPDATE LOGICAL STATE
    if (opc == OPC_GPON)  { g_TrackPower = true;  lastActivity = millis(); }
    if (opc == OPC_GPOFF) { g_TrackPower = false; }

    // DETECT ACTIVITY (Only relevant if Track Power is ON)
    if (g_TrackPower) {
        bool active = false;
        if (opc == OPC_SW_REQ) active = true;
        if (opc == OPC_LOCO_SPD && p->data[2] > 0) active = true;
        
        if (active) lastActivity = millis();
    }
}

bool ActivityMonitor::shouldTrigger() {
    // 1. Hardware Check: If system is dead, we can't do anything
    if (!g_SystemPower) return false;

    // 2. Logical Check: Only timeout if track is ON
    if (g_TrackPower && (millis() - lastActivity > timeoutMs)) {
        // We are triggering OFF. Update state immediately to prevent re-trigger
        g_TrackPower = false; 
        lastActivity = millis();
        return true;
    }
    return false;
}
