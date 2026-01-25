# LBServerWiFi v2.0.0 Technical Documentation

**Protocol Implementation: LBServer (LocoNet-over-TCP)**

---

## I. Executive Summary
LBServerWiFi v2.0.0 is a high-performance, multi-client bridge designed for the ESP32 microcontroller. It implements the **LBServer protocol** to enable simultaneous communication between multiple model railroad control applications and a physical LocoNet network. The defining advancement of this version is the **Synchronized Multi-Client Architecture**, which isolates timing-critical bit-processing from network overhead.

## II. Component Map

| Component | Files | Primary Responsibility |
| :--- | :--- | :--- |
| **AsyncDebug** | `AsyncDebug.h/cpp` | Non-blocking, thread-safe logging via FreeRTOS queues. |
| **WiFiManager** | `WiFiManager.h/cpp` | Manages WiFi connectivity, NVM credentials, and mDNS advertising. |
| **NetworkInterface** | `NetworkInterface.h/cpp` | Implements the **LBServer protocol** and manages the 3-slot client pool. |
| **LocoNet Hardware** | `LocoNetStreamESP32` | **External Dependency:** Handles low-level bit-timing for the LocoNet bus. |
| **Main Orchestrator** | `LBServerWiFi.ino` | Coordinates system startup and pins tasks to specific CPU cores. |

---

## III. System Architecture

### 1. Protocol Compatibility
* **LBServer Support**: This bridge implements the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver).* **WiThrottle Limitation**: **Note:** This bridge does **not** support the WiThrottle protocol directly. Mobile devices must connect via an app supporting LBServer or through a JMRI WiThrottle server.

### 2. Initialization & Tasking
* **Non-Blocking Startup**: The bridge begins processing LocoNet data immediately upon boot; it does not pause to wait for a initial network client.
* **Multicore Allocation**: To ensure deterministic timing, the `communicationTask` (Network I/O) is pinned to **Core 0**, while the `loop()` (LocoNet Hardware Timing) is reserved exclusively for **Core 1**.



### 3. Multi-Client Arbitration
* **Concurrent Capacity**: Supports up to 3 simultaneous TCP clients.
* **Global Echo Loopback**: Commands received from any single client are injected into the LocoNet stream and echoed back to all other connected network participants to ensure state synchronization.
* **Deterministic Cleanup**: The system utilizes explicit state tracking to instantly detect and prune "Half-Open" TCP connections when a client application is closed, ensuring slot availability without waiting for TCP timeouts.

---

## IV. Usage and Operation
* **WiFi Setup**: If no credentials exist, connect to the `LBServer` AP and navigate to `http://192.168.4.1`.
* **JMRI Configuration**:
    * **System Manufacturer**: Digitrax.
    * **System Connection**: LocoNet over TCP.
    * **Hostname**: `LBServer.local` (Port 1234).
    * **Command Station**: Intellibox-I (only one tested).
* **Remote Debugging**: Real-time wireless logs can be viewed by running `telnet LBServer.local` from a terminal.

---

## V. Acknowledgments & Credits
* **Stefan Bormann**: A sincere thank you to the author of the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver) for providing the foundation for this bridge.
* **LocoNet Hardware Interface**: This project utilizes `LocoNetStreamESP32` library for low-level hardware communication.
* **Development Partner**: The asynchronous architecture and multi-client implementation in this version were developed in collaboration with **Google Gemini**.
