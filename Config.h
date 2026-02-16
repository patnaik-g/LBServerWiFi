/**
 * @file Config.h
 * @brief Global Configuration & Hardware Pinout
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// 1. FIRMWARE IDENTITY
// =============================================================================
#define BRIDGE_VERSION "2.5.4"

// =============================================================================
// 2. FEATURE FLAGS (COMPILE-TIME OPTIONS)
// =============================================================================
#define SYSTEM_POWER_CONTROL 

// =============================================================================
// 3. SAFETY TIMEOUTS (MILLISECONDS)
// =============================================================================
#define TIMEOUT_TRACK_MS 900000   
#define TIMEOUT_SYSTEM_MS 1800000 

// =============================================================================
// 4. HARDWARE PINOUT (ESP32)
// =============================================================================
#define PIN_LOCONET_RX 26
#define PIN_LOCONET_TX 18
#define PIN_POWER_MONITOR 33
#define PIN_STATUS_LED 2

// =============================================================================
// 5. NETWORK SETTINGS
// =============================================================================
#define DEFAULT_HOSTNAME "lbserver"
#define DEFAULT_PORT 1234
#define MAX_CLIENTS 3
#define MDNS_SERVICE_NAME "loconetovertcpserver"
#define MDNS_SERVICE_PROTO "tcp"

// =============================================================================
// 6. SMART PLUG INTEGRATION
// =============================================================================
#define KASA_SMARTPLUG_NAME "Layout"

// =============================================================================
// 7. ADVANCED DEBUGGING & TUNING
// =============================================================================
//#define ENABLE_SERIAL_LOGGING
#define ENABLE_TELNET_LOGGING
//#define ENABLE_HEAP_MONITOR // Needed to check heap

#define SERIAL_BAUD_RATE 115200
#define LOCONET_QUEUE_DEPTH 64
#define DEBUG_QUEUE_DEPTH 64
#define DEBUG_MSG_LEN 256

#endif
