# LBServerWiFi v2.2.0 Technical Documentation

**Protocol Implementation: LBServer (LocoNet-over-TCP)**

---

## I. Executive Summary
LBServerWiFi v2.2.0 is a high-performance, multi-client bridge designed for the ESP32 microcontroller. It implements the **LBServer protocol** to enable simultaneous communication between multiple model railroad control applications and a physical LocoNet network. This version marks a transition to a **Decoupled Domain Architecture**, where network transport is strictly isolated from protocol logic through functional abstraction and full encapsulation.

## II. Component Map

| Component | Files | Primary Responsibility |
| :--- | :--- | :--- |
| **AsyncDebug** | `AsyncDebug.h/cpp` | Non-blocking telemetry via a globally-linked (`extern`) FreeRTOS queue. |
| **WiFiManager** | `WiFiManager.h/cpp` | Transport Layer: Manages the TCP stack, mDNS, and OTA. Encapsulates the client pool as `private` data. Includes Serial Boot Menu. |
| **NetworkInterface** | `NetworkInterface.h/cpp` | Protocol Layer: Processes LBServer logic via a functional iterator. Hosts optimized hex/buffer helpers and global configuration. |
| **LocoNet Hardware** | `LocoNetStreamESP32` | Handles low-level bit-timing for the physical LocoNet bus. |
| **Power Monitor** | `PowerLine.h/cpp` | **New**: Dedicated low-priority FreeRTOS task for hardware debouncing and atomic state management. |
| **Main Orchestrator** | `LBServerWiFi.ino` | Coordinates system startup and pins tasks to CPU cores. |

---

## III. System Architecture

### 1. Protocol Compatibility
* **LBServer Support**: This bridge implements the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver).

### 2. Initialization & Tasking
* **Non-Blocking Startup**: The bridge begins processing LocoNet data immediately upon boot.
* **Multicore Allocation**: `communicationTask` is pinned to **Core 0**, while LocoNet is reserved for **Core 1**.
* **Threaded Power Monitoring**: Power sensing runs in a dedicated, low-priority task.
