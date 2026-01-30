# LBServerWiFi v2.4.0

**A high-performance, asynchronous WiFi-to-LocoNet bridge for the ESP32 implementing the LBServer protocol.**

## Summary
LBServerWiFi enables model railroad control software (e.g., JMRI, RocRail) to communicate with a LocoNet network via TCP/IP. This project provides a robust implementation of the **LBServer protocol**, utilizing a dual-core FreeRTOS architecture to separate timing-critical LocoNet bit-processing from network handling and asynchronous logging.

## Key Features
* **LBServer Implementation**: Support for the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver) (LocoNet-over-TCP) to ensure seamless integration with modern control software.
* **Multi-Client Concurrency**: Synchronized support for up to 3 simultaneous clients, allowing multiple instances of JMRI to operate.
* **OTA Updates**: Integrated Over-the-Air (OTA) support for wireless firmware updates when the system is idle.
* **Dual-Core Task Isolation**: Strategic pinning of time-critical LocoNet bit-processing to Core 1, while handling all network I/O and logging on Core 0.
* **Global State Synchronization**: An automated system that broadcasts commands from any sngle client to all connected clients.
* **mDNS Discovery**: Automatic network advertising as `LBServer.local`, eliminating the need for manual IP configuration.
* **Web-Based WiFi Provisioning**: A built-in Access Point (AP) mode and captive portal for easy configuration of network credentials.
* **Wireless and Serial Debugging**: Asynchronous diagnostic monitoring on serial port and via Telnet clients using a idle-priority task.
* **Automatic Idle Power-Off**: Monitors LocoNet and WiFi activity to automatically turn off track power after 15 minutes of inactivity.
* **Optional System Power Control**: Integrates with Kasa Smart Plugs to cut physical system power after exteded inactivity.

## Documentation
For a detailed technical breakdown of the component architecture, safety systems, and compilation options, please refer to the [LBServerWiFi_Docs.md](./LBServerWiFi_Docs.md) file.

## Acknowledgments & Credits
* **Protocol Design**: A sincere thank you to **Stefan Bormann**, the author of the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver), for providing the foundation for this bridge.
* **Development Partner**: The asynchronous architecture, multi-client logic, and performance-tuned implementation in this version were developed in collaboration with **Google Gemini**.
