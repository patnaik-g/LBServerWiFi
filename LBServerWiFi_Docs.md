# LBServerWiFi v2.0.1 Technical Documentation

**Protocol Implementation: LBServer (LocoNet-over-TCP)**

---

## I. Executive Summary
LBServerWiFi v2.0.1 is a high-performance, multi-client bridge designed for the ESP32 microcontroller. It implements the **LBServer protocol** to enable simultaneous communication between multiple model railroad control applications and a physical LocoNet network. The defining advancement of this version is the **Synchronized Multi-Client Architecture**, which isolates timing-critical bit-processing from network overhead and supports **wireless OTA maintenance**.

## II. Component Map

| Component | Files | Primary Responsibility |
| :--- | :--- | :--- |
| **AsyncDebug** | `AsyncDebug.h/cpp` | Non-blocking, thread-safe logging via FreeRTOS queues. |
| **WiFiManager** | `WiFiManager.h/cpp` | Manages WiFi connectivity and NVM credentials. |
| **NetworkInterface** | `NetworkInterface.h/cpp` | Implements the **LBServer protocol** and manages the 3-slot client pool. |
| **LocoNet Hardware** | `LocoNetStreamESP32` | Handles low-level bit-timing for the LocoNet bus. |
| **Main Orchestrator** | `LBServerWiFi.ino` | Coordinates system startup and pins tasks to CPU cores. |

---

## III. System Architecture

### 1. Protocol Compatibility
* **LBServer Support**: This bridge implements the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver).
* **WiThrottle Limitation**: Mobile devices must connect via an app supporting LBServer or through a JMRI WiThrottle server.

### 2. Initialization & Tasking
* **Non-Blocking Startup**: The bridge begins processing LocoNet data immediately upon boot.
* **Multicore Allocation**: `communicationTask` is pinned to **Core 0**, while LocoNet timing is reserved for **Core 1**.

### 3. Multi-Client Arbitration
* **Concurrent Capacity**: Supports up to 3 simultaneous TCP clients.
* **Global Echo Loopback**: Commands are echoed back to all clients to ensure state synchronization.
* **Deterministic Cleanup**: Explicit state tracking identifies and prunes "Half-Open" sockets immediately.

### 4. Maintenance (OTA)
* **Wireless Updates**: The bridge supports Over-the-Air updates. To ensure protocol stability, the OTA handler is only active when no network clients are connected.

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

## VI. Change Log

| Version | Date | Changes |
| :--- | :--- | :--- |
| **v2.0.1** | 2026-01-26 | Restored OTA functionality; removed dead code from `WiFiManager`; optimized idle LED logic. |
| **v2.0.0** | 2026-01-25 | Initial multi-client release; dual-core task isolation; LBServer implementation. |
