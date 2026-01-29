# LBServerWiFi v2.3.0 Technical Documentation

**Protocol Implementation: LBServer (LocoNet-over-TCP)**

---

## I. Executive Summary
LBServerWiFi v2.3.0 is a high-performance, multi-client bridge designed for the ESP32 microcontroller. It implements the **LBServer protocol** to enable simultaneous communication between multiple model railroad control applications and a physical LocoNet network. This version marks a transition to a **Decoupled Domain Architecture**, where network transport is strictly isolated from protocol logic through functional abstraction and full encapsulation. It also introduces an **Automatic Idle Safety System** to enforce power-saving policies.

## II. Component Map

| Component | Files | Primary Responsibility |
| :--- | :--- | :--- |
| **AsyncDebug** | `AsyncDebug.h/cpp` | Non-blocking telemetry via a globally-linked (`extern`) FreeRTOS queue. |
| **WiFiManager** | `WiFiManager.h/cpp` | Transport Layer: Manages the TCP stack, mDNS, and OTA. Encapsulates the client pool as `private` data. Includes Serial Boot Menu. |
| **NetworkInterface** | `NetworkInterface.h/cpp` | Protocol Layer: Processes LBServer logic via a functional iterator. Hosts optimized hex/buffer helpers and global configuration. |
| **LocoNet Hardware** | `LocoNetStreamESP32` | Handles low-level bit-timing for the physical LocoNet bus. |
| **Power Monitor** | `PowerLine.h/cpp` | **Hardware Layer**: Dedicated low-priority FreeRTOS task for hardware debouncing and atomic state management. |
| **Activity Monitor** | `ActivityMonitor.h/cpp` | **Logic Layer**: Tracks LocoNet and WiFi packets to enforce a 15-minute idle timeout policy for Track Power. |
| **Main Orchestrator** | `LBServerWiFi.ino` | Coordinates system startup and pins tasks to CPU cores. |

---

## III. System Architecture

### 1. Protocol Compatibility
* **LBServer Support**: This bridge implements the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver).

### 2. Initialization & Tasking
* **Non-Blocking Startup**: The bridge begins processing LocoNet data immediately upon boot.
* **Multicore Allocation**: `communicationTask` is pinned to **Core 0**, while LocoNet is reserved for **Core 1**.
* **Threaded Power Monitoring**: Power sensing runs in a dedicated, low-priority task.

### 3. Safety Systems
* **System vs. Track Power**: The system distinguishes between hardware bus power (`g_SystemPower`) and logical rail power (`g_TrackPower`).
* **Idle Timeout**: If `g_TrackPower` is active and no user commands (Switch, Speed, Power) are detected for 15 minutes, the system automatically sends `GP_OFF`.

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
| **v2.3.0** | 2026-01-29 | **Idle Safety System**: Added `ActivityMonitor` and `PowerLine` integration to enforce 15-minute track power timeout. Updated to `g_SystemPower` gating. |
| **v2.2.0** | 2026-01-28 | **Decoupled Architecture**: Refactored transport logic into `WiFiManager` and protocol logic into `NetworkInterface`. Added `std::function` callbacks for strict encapsulation. |
| **v2.1.0** | 2026-01-26 | **Architectural Refactor**: Fully encapsulated TCP client pool (moved to `private`); implemented `forEachActiveClient` functional iterator; stabilized `AsyncDebug` via `extern` linkage. |
| **v2.0.1** | 2026-01-26 | Restored OTA functionality; removed dead code from `WiFiManager`; optimized idle LED logic. |
| **v2.0.0** | 2026-01-25 | Initial multi-client release; dual-core task isolation; LBServer implementation. |
