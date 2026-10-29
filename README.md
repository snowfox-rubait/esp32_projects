# ESP32 Wireless UART Bridge & Control Panel

An advanced, high-fidelity wireless serial console adapter firmware for the ESP32. It turns your ESP32 into a smart UART-to-Wi-Fi and UART-to-USB bridge, allowing you to wireless monitor and control connected serial devices (like router consoles, switches, or other microcontrollers) from a premium glassmorphic dark-mode web panel.

---

## Features

- **Double-Buffered Serial Pass-through**: Low-latency bi-directional data flow between USB (PC) $\leftrightarrow$ ESP32 UART2 (External Device) $\leftrightarrow$ Web Console.
- **Glassmorphic Web Dashboard**: Beautiful, real-time reactive dark-mode web console served directly from the ESP32 memory.
- **Wi-Fi Manager with 30s Fallback**:
  - Automatically boots in **Access Point (AP) Mode** (`SSID: ESP32_UART_Bridge`) out-of-the-box.
  - Allows saving your local Wi-Fi router credentials from the Web UI.
  - Tries to connect to the saved Wi-Fi on boot. If the connection fails or takes longer than **30 seconds**, it automatically falls back to AP mode so you never lose control.
- **Dynamic Pin Swapping**: Swap TX/RX pins dynamically at any time via the Web UI or USB Commands without reflashing.
- **Baud Rate Manager**:
  - Predefined presets (9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600).
  - Custom Baud Rate field to input any arbitrary baud value.
- **Persistent Memory**: Saves your Wi-Fi credentials, baud rate, and pin mapping to the ESP32's Preferences flash namespace, restoring them automatically across reboots.

---

## Hardware Pin Mapping (Safe Pins)

To avoid conflict with ESP32 bootstrapping pins (like GPIO 0 and GPIO 2 which can put the chip into bootloader mode), this firmware defaults to using standard non-boot pins:

| ESP32 Pin | Default Connection | Router Serial Header |
|-----------|--------------------|----------------------|
| **GPIO 16** | ESP32 RX2          | Router TX            |
| **GPIO 17** | ESP32 TX2          | Router RX            |
| **GND**     | Common Ground      | Router GND           |

*Note: Never connect VCC/Power pins between the ESP32 and the target device.*

---

## Local USB Control Commands

If you are connected via standard serial terminal (e.g. `picocom -b 115200 /dev/ttyUSB0`), you can type these local commands to configure the bridge directly:

- `CMD:STATUS` : Query current configuration (baud rate, pinout, Wi-Fi mode).
- `CMD:SWAP` : Dynamically swap RX and TX pins (GPIO 16 $\leftrightarrow$ GPIO 17).
- `CMD:BAUD:<value>` : Set a custom baud rate (e.g., `CMD:BAUD:115200` or `CMD:BAUD:57600`).
- `CMD:WIFI:<ssid>:<password>` : Save Wi-Fi credentials and reboot.

---

## Directory Structure

```text
~/Work/esp32/codes/
├── README.md                 # This documentation
└── UartBridge/
    └── UartBridge.ino        # Full Arduino C++ Source Code
```
