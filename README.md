# LBServerWiFi v1.8.0

**A high-performance, asynchronous WiFi-to-LocoNet bridge for the ESP32 implementing the LBServer protocol, ensuring diagnostic logging never interrupts time-critical model railroad control.**

## Summary
LBServerWiFi enables model railroad control software (e.g., JMRI) to communicate with a LocoNet network via TCP/IP. This project provides a robust implementation of the **LBServer protocol**, utilizing a dual-core FreeRTOS architecture to separate timing-critical LocoNet bit-processing from network handling and asynchronous logging.

## Key Features
* **Protocol Implementation**: Full support for the LBServer protocol for seamless integration with JMRI and other LocoNet-over-TCP clients.
* **Asynchronous Logging**: Diagnostic output is handled by a background idle-priority task to prevent blocking the main loop.
* **mDNS/Bonjour**: Automatically advertised as `LBServer.local` for easy discovery.
* **Wireless Debugging**: Real-time log monitoring via Telnet (`telnet LBServer.local`).
* **Web Configuration**: Built-in AP mode and web portal for managing WiFi credentials in NVM.

## Documentation
For a detailed technical breakdown of the component architecture, integration flow, and file structure, please refer to the [LBServerWiFi_Docs.rtf](./LBServerWiFi_Docs.rtf) file included in this repository.

## Acknowledgments & Credits
* **Original Author**: A sincere thank you to the author of the LBServer protocol for providing the foundation for this bridge.
* **Development Partner**: The asynchronous architecture and refactored C++/Fortran-style logic in this version were developed in collaboration with **Google Gemini**.
