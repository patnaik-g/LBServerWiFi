/*
 * @file Config.h
 * @brief Global Configuration & Hardware Pinout
 * * USER GUIDE:
 * This file contains all the settings for the LBServerWiFi Bridge.
 * Edit the values below to match your specific layout hardware and preferences.
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// 1. FIRMWARE IDENTITY
// =============================================================================
// The version string broadcast to clients (e.g., JMRI) upon connection.
#define BRIDGE_VERSION "2.4.0"


// =============================================================================
// 2. FEATURE FLAGS (COMPILE-TIME OPTIONS)
// =============================================================================
// Uncomment the line below to enable "Tier 2" safety.
// ENABLED:  The system will scan for a Kasa Smart Plug and cut power after 30 mins.
// DISABLED: The system relies only on Tier 1 (Track Power Off) safety.
//           Dependencies on KasaSmartPlug library are removed to save space.
#define SYSTEM_POWER_CONTROL 


// =============================================================================
// 3. SAFETY TIMEOUTS (MILLISECONDS)
// =============================================================================
// TIER 1: LOGICAL TRACK POWER
// If no throttle/switch commands are received for this duration, the bridge
// sends a LocoNet 'GP_OFF' command to stop all trains.
// Default: 15 Minutes (15 * 60 * 1000 = 900000)
#define TIMEOUT_TRACK_MS 900000   

// TIER 2: PHYSICAL SYSTEM POWER (Requires SYSTEM_POWER_CONTROL)
// If the system remains idle for this duration, the bridge sends a command
// to the Smart Plug to cut AC power to the entire layout.
// Default: 30 Minutes (30 * 60 * 1000 = 1800000)
#define TIMEOUT_SYSTEM_MS 1800000 


// =============================================================================
// 4. HARDWARE PINOUT (ESP32)
// =============================================================================
// RX/TX Pins for the LocoNet Interface (via Optocoupler/Transistor)
#define PIN_LOCONET_RX 22
#define PIN_LOCONET_TX 23

// Input Pin for Power Monitor circuit (Detects if Rails are ON/OFF)
#define PIN_POWER_MONITOR 34

// Status LED (Onboard LED usually Pin 2)
// Steady ON = Idle/Ready. Blinking = Active Client Connected.
#define PIN_STATUS_LED 2


// =============================================================================
// 5. NETWORK SETTINGS
// =============================================================================
// The hostname used for mDNS discovery (e.g., http://lbserver.local)
#define DEFAULT_HOSTNAME "lbserver"

// The TCP port that clients (JMRI, RocRail) connect to.
// Standard LBServer port is 1234.
#define DEFAULT_PORT 1234

// Maximum number of simultaneous TCP clients allowed.
#define MAX_CLIENTS 3

// mDNS Service Advertisements (Do not change unless protocol changes)
#define MDNS_SERVICE_NAME "loconetovertcpserver"
#define MDNS_SERVICE_PROTO "tcp"


// =============================================================================
// 6. SMART PLUG INTEGRATION
// =============================================================================
// The name of the TP-Link Kasa Smart Plug to control.
// This must EXACTLY match the name assigned in the Kasa Mobile App.
#define KASA_SMARTPLUG_NAME "Layout"


// =============================================================================
// 7. ADVANCED DEBUGGING & TUNING
// =============================================================================
// Serial Port Speed (Baud Rate)
#define SERIAL_BAUD_RATE 115200

// Depth of the LocoNet Message Queues (Traffic Buffers).
// WARNING: Only modify this if you are experiencing specific performance issues.
// - Increase (e.g., 64) if you see dropped packets during massive automation bursts.
// - Decrease (e.g., 16) ONLY if you are running out of Heap Memory (RAM) crashes.
// Default: 32 (Approx 500 bytes per queue; Balanced for ESP32)
#define LOCONET_QUEUE_DEPTH 32

// Depth of the FreeRTOS queue for the AsyncDebug engine.
#define DEBUG_QUEUE_DEPTH 32

// Maximum length of a single debug log message (in bytes).
#define DEBUG_MSG_LEN 256

#endif
