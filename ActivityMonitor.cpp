#include "ActivityMonitor.h"
#include "LocoNetPackets.h"
#include "AsyncDebug.h"

// The standard message length for a LocoNet Slot Read/Write Data packet is 14 bytes.
const uint8_t LNSLOT_MSG_SIZE = 0x0E;

// --- 1. FEATURE IMPLEMENTATION (Enabled/Disabled Variants) ---

#ifdef ENABLE_KASA_CONTROL
#include "KasaSmartPlug.h"

// [ENABLED] Constructor & Setup
ActivityMonitor::ActivityMonitor(uint32_t trackTimeout, uint32_t systemTimeout)
  : lastActivity(millis()), trackTimeoutMs(trackTimeout), systemTimeoutMs(systemTimeout),
    _lnStream(nullptr), _lnToNetQueue(nullptr), _plug(nullptr) {
#ifdef ENABLE_POWER_MONITOR
  _wasSystemOff = false;
#endif
}

void ActivityMonitor::begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue, KasaPlug* plug) {
  _lnStream = lnStream;
  _lnToNetQueue = lnToNetQueue;
  _plug = plug;
}

// [ENABLED] Helper Logic
bool ActivityMonitor::checkSystemTimeout(uint32_t now) {
#ifdef ENABLE_POWER_MONITOR
  if (!g_SystemPower) return false;
#endif
  return (now - lastActivity > systemTimeoutMs);
}

void ActivityMonitor::performSystemShutdown() {
  // We must reset the activity timer as soon as we begin the shutdown process.
  // This prevents the manage() loop from repeatedly calling this function
  // while the (slow) network operation to the Kasa plug is in progress.
  reset();
  LOG_DEBUG("Idle Timeout (%d min). System Power OFF.\n", systemTimeoutMs / 60000);
  if (_plug) {
    _plug->attemptShutdown();
  } else {
    LOG_DEBUG("Error: No Kasa Plug configured.\n");
  }
}

#else

// [DISABLED] Constructor & Setup
ActivityMonitor::ActivityMonitor(uint32_t trackTimeout)
  : lastActivity(millis()), trackTimeoutMs(trackTimeout),
    _lnStream(nullptr), _lnToNetQueue(nullptr) {
#ifdef ENABLE_POWER_MONITOR
  _wasSystemOff = false;
#endif
}

void ActivityMonitor::begin(LocoNetStreamESP32* lnStream, QueueHandle_t lnToNetQueue) {
  _lnStream = lnStream;
  _lnToNetQueue = lnToNetQueue;
}

// [DISABLED] Stubs (Optimized away by compiler)
bool ActivityMonitor::checkSystemTimeout(uint32_t now) {
  return false;
}
void ActivityMonitor::performSystemShutdown() { /* No-Op */
}

#endif


// --- 2. COMMON LOGIC ---

#ifdef ENABLE_POWER_MONITOR
bool ActivityMonitor::isSystemOff() {
  if (!g_SystemPower) {
    _wasSystemOff = true;
    return true;
  }
  return false;
}
#endif

void ActivityMonitor::reset() {
  lastActivity = millis();
}

void ActivityMonitor::inspect(const lnMsg* p) {
  uint8_t opc = p->data[0];
  uint32_t now = millis();
  if (opc == OPC_GPON) {
    g_TrackPower = true;
    lastActivity = now;
  }
  if (opc == OPC_GPOFF) {
    g_TrackPower = false;
  }
  if ((opc == OPC_WR_SL_DATA || opc == OPC_SL_RD_DATA) && p->data[1] == LNSLOT_MSG_SIZE) {
    g_TrackPower = (p->data[7] & 0x01) != 0;
    LOG_DEBUG("Track power %s (slot %d)\n", g_TrackPower ? "ON" : "OFF", p->data[2]);
    if (g_TrackPower) lastActivity = now;
  }

  if (g_TrackPower) {
    bool active = false;
    if (opc == OPC_SW_REQ) active = true;
    if (opc == OPC_LOCO_SPD && p->data[2] > 0) active = true;
    if (active) lastActivity = now;
  }
}

bool ActivityMonitor::shouldTriggerTrackOff(uint32_t now) {
#ifdef ENABLE_POWER_MONITOR
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
#ifdef ENABLE_POWER_MONITOR
  if (_wasSystemOff) {
    if (g_SystemPower) {
      LOG_DEBUG("System Power Restored. Resetting Idle Timer.\n");
      lastActivity = now;
      _wasSystemOff = false;
#ifdef ENABLE_KASA_CONTROL
      if (_plug) {
        _plug->resetShutdownLatch();
      }
#endif
    }
  }
#endif

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
