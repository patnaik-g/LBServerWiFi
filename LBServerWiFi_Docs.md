# LBServerWiFi Technical Documentation

**Protocol Implementation: LBServer (LocoNet-over-TCP)**

---

## I. Executive Summary
LBServerWiFi is a high-performance, multi-client bridge designed for the ESP32 microcontroller. It implements the **LBServer protocol** to enable simultaneous communication between multiple model railroad control applications and a physical LocoNet network. This version features a **Decoupled Domain Architecture** and a **Configurable Idle Safety System** that allows site-specific power management hardware to be enabled or disabled at compile time via feature flags.

## II. Component Map

| Component | Files | Primary Responsibility |
| :--- | :--- | :--- |
| **AsyncDebug** | `AsyncDebug.h/cpp` | Non-blocking telemetry via a globally-linked (`extern`) FreeRTOS queue. |
| **WiFiManager** | `WiFiManager.h/cpp` | Transport Layer: Manages the TCP stack, mDNS, and OTA. Encapsulates the client pool as `private` data. Includes Serial Boot Menu. |
| **NetworkInterface** | `NetworkInterface.h/cpp` | Protocol Layer: Processes LBServer logic via a functional iterator. Hosts optimized hex/buffer helpers and global configuration. |
| **LocoNet Hardware** | `LocoNetStreamESP32` | Handles low-level bit-timing for the physical LocoNet bus. |
| **UART Tuning** | `UartTuning.h` | **Hardware Layer**: Configures low-level UART parameters (glitch filter, RX timeout) and provides a hardware error watchdog for bus stability. |
| **Power Monitor** | `PowerLine.h/cpp` | **Hardware Layer**: Dedicated low-priority FreeRTOS task for hardware debouncing and atomic state management. |
| **Activity Monitor** | `ActivityMonitor.h/cpp` | **Logic Layer**: Tracks LocoNet and WiFi packets to enforce idle timeout policies. |
| **Main Orchestrator** | `LBServerWiFi.ino` | Coordinates system startup and pins tasks to CPU cores. |

---

## III. System Architecture

### 1. Protocol Compatibility
* **LBServer Support**: This bridge implements the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver).

### 2. Initialization & Tasking
* **Non-Blocking Startup**: The bridge begins processing LocoNet data immediately upon boot.
* **Multicore Allocation**: `communicationTask` is pinned to **Core 0**, while LocoNet is reserved for **Core 1**.
* **Threaded Power Monitoring**: Power sensing runs in a dedicated, low-priority task.

### 3. Safety Systems (Tiered Protection)
The bridge implements a dual-stage safety mechanism to prevent unattended operation:
* **Tier 1: Track Power (15 Minutes)**
    * **Trigger**: 15 minutes of inactivity (no Speed, Switch, or Power commands).
    * **Action**: Sends `OPC_GPOFF` to the LocoNet bus.

* **Tier 2: System Power (30 Minutes)**
    * **Trigger**: 30 minutes of inactivity.
    * **Action**: Sends a command to a specific TP-Link Kasa Smart Plug (Alias: "Layout") to cut physical power to the entire layout.
    * **Optional** (See *Configuration* below).

### 4. Configuration & Conditional Compilation
To support diverse hardware setups, features are controlled via compile-time feature flags in `Config.h`:

* **`#define ENABLE_POWER_MONITOR`**
    * **Enabled**: Compiles the `PowerLine` hardware monitor task to detect the physical layout power state and synchronize it with network clients.
    * **Disabled (Commented out)**: Excludes the power monitoring task, allowing the bridge to run independently of the layout's physical power state.

* **`#define ENABLE_KASA_CONTROL`**
    * **Enabled**: The firmware compiles with the `KasaSmartPlug` library for "Tier 2" protection. It will scan for the smart plug during boot and enforce the 30-minute system timeout.
    * **Disabled (Commented out)**: All Kasa-related code and dependencies are excluded from the build. The system reduces binary size and relies solely on Tier 1 (Track Power) safety. This ensures the code remains portable for users without specific smart plug hardware.

---

## IV. Usage and Operation
* **WiFi Setup**: If no credentials exist, connect to the `LBServer` AP at `http://192.168.4.1`.
* **JMRI Configuration**:
    * **System Manufacturer**: Digitrax.
    * **System Connection**: LocoNet over TCP.
    * **Hostname**: `LBServer.local` (Port 1234).
* **Remote Debugging**: Wireless logs available via `telnet LBServer.local`.

---

## V. Acknowledgments & Credits
* **Stefan Bormann**: Author of the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver).
* **LocoNet Hardware Interface**: Utilizes `LocoNetStreamESP32` library.
* **Development Partner**: Collaborative architecture developed with **Google Gemini**.

---

## VI. Version History

| Version | Date | Changes |
| :--- | :--- | :--- |
| **v2.6.0** | 2026-04-30 | **Hardware-Level UART Tuning & Logic Refinement**: Introduced `UartTuning.h` to apply low-level hardware settings to the LocoNet UART, including a glitch filter and RX FIFO timeout for improved bus stability. Added a hardware error watchdog to detect and log bus issues. |
| **v2.5.5** | 2026-03-29 | **Traffic Gating & Logging**: Implemented software-level gating for incoming LocoNet traffic to suppress noise from unpowered bus. Added explicit incoming packet logging to debug console. Optimized broadcast and LED pulses to trigger only when network clients are active. Refined PowerLine task for atomic power state synchronization. |
| **v2.5.4** | 2026-02-15 | **Memory Stability & Headless Optimization**: Fixed overnight heap fragmentation via "Silent Keep-Alive" logic. Moved critical network and JSON buffers to static memory (BSS) to prevent stack overflow. Optimized for headless boot (instant startup without Serial) and added 10-minute heap health telemetry. Corrected hardware pinout and LocoNet opcodes. |
| **v2.5.2** | 2026-02-02 | **Traffic LED Logic**: Implemented "Solid Idle / Active Pulse" logic. LED stays solid ON when 0 clients are connected. Pulses OFF/ON (10ms) for traffic or 10s heartbeat when active. |
| **v2.5.1** | 2026-02-01 | **Portal Fix**: Restored functional WiFi Setup Portal with HTML input forms. Added `viewport` meta tag for mobile scaling. Fixed hardcoded hostname in AP Mode logs and web headers. |
| **v2.5.0** | 2026-01-31 | **Configuration & Stability**: Introduced `Config.h` to consolidate global settings and feature flags. Locked `DebugTask` stack to 16KB to prevent Telnet overflows. Fixed mDNS startup race condition. |
| **v2.4.0** | 2026-01-30 | **Configurable Power Control**: Added `SYSTEM_POWER_CONTROL` compile-time switch. Implemented optional **30-minute System Power Timeout** using Kasa Smart Plug integration. Refactored `ActivityMonitor` to use private helper stubs for clean conditional logic. |
| **v2.3.0** | 2026-01-29 | **Idle Safety System**: Added `ActivityMonitor` and `PowerLine` integration to enforce 15-minute track power timeout. Updated to `g_SystemPower` gating. |
| **v2.2.0** | 2026-01-28 | **Decoupled Architecture**: Refactored transport logic into `WiFiManager` and protocol logic into `NetworkInterface`. Added `std::function` callbacks for strict encapsulation. |
| **v2.1.0** | 2026-01-26 | **Architectural Refactor**: Fully encapsulated TCP client pool (moved to `private`); implemented `forEachActiveClient` functional iterator; stabilized `AsyncDebug` via `extern` linkage. |
| **v2.0.1** | 2026-01-26 | Restored OTA functionality; removed dead code from `WiFiManager`; optimized idle LED logic. |
| **v2.0.0** | 2026-01-25 | Initial multi-client release; dual-core task isolation; LBServer implementation. |
