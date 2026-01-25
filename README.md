# LBServerWiFi v2.0.0

**A high-performance, asynchronous WiFi-to-LocoNet bridge for the ESP32 implementing the LBServer protocol.**

## Summary
LBServerWiFi enables model railroad control software (e.g., JMRI, RocRail) to communicate with a LocoNet network via TCP/IP. This project provides a robust implementation of the **LBServer protocol**, utilizing a dual-core FreeRTOS architecture to separate timing-critical LocoNet bit-processing from network handling and asynchronous logging.

## Key Features
* **Full LBServer Implementation**: Support for the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver) (LocoNet-over-TCP) to ensure seamless integration with modern control software.
* **Multi-Client Concurrency**: Synchronized support for up to 3 simultaneous TCP connections, allowing instances of JMRI to operate at once.
* **Dual-Core Task Isolation**: Strategic pinning of time-critical LocoNet bit-processing to Core 1, while handling all network I/O and logging on Core 0.
* **Deterministic Session Cleanup**: High-reliability disconnect logic that instantly identifies and frees "Half-Open" TCP sockets when a client application is closed.
* **Zero-Wait Startup**: A non-blocking initialization phase that allows the bridge to process LocoNet data immediately upon boot without waiting for a network client.
* **Asynchronous Diagnostic Logging**: A thread-safe logging system that uses a background idle-priority task.
* **Global State Synchronization**: An automated echo loopback system that broadcasts commands from any single client to all other connected participants.
* **mDNS Discovery**: Automatic network advertising as `LBServer.local`, eliminating the need for manual IP configuration.
* **Web-Based WiFi Provisioning**: A built-in Access Point (AP) mode and captive portal for easy configuration of network credentials.
* **Wireless and Serial Debugging**: Real-time diagnostic monitoring accessible via standard Telnet clients (`telnet LBServer.local`).

## Documentation
For a detailed technical breakdown of the component architecture and multi-client arbitration, please refer to the [LBServerWiFi_Docs.md](./LBServerWiFi_Docs.md) file.

## Acknowledgments & Credits
* **Protocol Design**: A sincere thank you to **Stefan Bormann**, the author of the [LBServer protocol](https://loconetovertcp.sourceforge.net/Server/index.html#lbserver), for providing the foundation for this bridge.
* **Development Partner**: The asynchronous architecture, multi-client logic, and performance-tuned implementation in this version were developed in collaboration with **Google Gemini**.
