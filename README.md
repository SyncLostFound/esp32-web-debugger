# ESP32 Web Debugger

A full-featured web-based GPIO and serial debugger for ESP32 boards. This tool provides real-time control and monitoring of digital pins, PWM output, analog input (oscilloscope-style), VIN voltage display, UART terminal, and I2C scanning — all through a sleek WebSocket-connected browser interface.

## 🚨 Important Note

This project was originally started some time ago using the **ESP32 Arduino Core version 2.0.17**, which was stable at the time and supported the web server and WebSocket features used here. Some libraries and approaches (like `ESPAsyncWebServer`) may now be outdated or deprecated in newer versions.

 **requires ESP32 Arduino core version `2.0.17` or `2.0.18`** to work correctly. The built-in async web libraries, file system mounting, and WebSocket handling are not guaranteed to work as expected in earlier or later versions due to API changes.

> **⚠️ Status: Currently in development / buggy.**  
> Some features like pin dropdowns and PWM input fields may refresh too often or cause UI jank. Known rendering issues are being refined.
>
> ⚠️ **Please note**:
- The project is still under active development.
- I plan to migrate this codebase to use the **latest stable ESP32 Arduino core**, along with more modern and officially supported **web libraries** (such as `WebServer.h`, `AsyncWebSocket`, or `esp_websocket_server` from the ESP-IDF).
- Future updates will also support **Arduino IDE 2.x**, improved frontend modularity, and a full WebSocket abstraction layer.

You're welcome to clone and experiment with the current version, but expect minor bugs and performance issues due to version quirks in `2.0.17/18`.


---

## 💻 Features

- 🖥️ **Live GPIO control:** Input/output mode toggle, state display, and real-time pin write.
- 🌈 **PWM frequency adjustment** per pin (1 Hz – 10,000 Hz).
- 📡 **Oscilloscope-style analog graph** for selected ADC pins (GPIO34, GPIO35, GPIO36, GPIO39).
- 🔋 **VIN monitor** showing input voltage once per second.
- 🧠 **Debounce counter** for each GPIO pin input (shows how many bounces detected).
- 💬 **UART serial terminal** — send/receive raw data.
- 🔎 **I2C bus scanner** — detects connected I2C devices and shows addresses.
- 📑 **WebSocket-based communication** — no page reloads, low-latency updates.
- ⚡ **Minimalist dark UI** with responsive layout.

---

## 📦 Requirements

### Hardware

- Any ESP32 board with WiFi support
- GPIO pins accessible for digital I/O, analog, and optional UART/I2C usage

### Software

- [Arduino IDE](https://www.arduino.cc/en/software) with **ESP32 board package version `2.0.17` or `2.0.18`**
- SPIFFS file system enabled and initialized
- `ESPAsyncWebServer` and `AsyncTCP` libraries

---

## 🔧 Uploading & Setup

1. **Install ESP32 Arduino Core `2.0.18`:**
   - Use the board manager URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Select your ESP32 board and install version `2.0.18`.

2. **Install required libraries:**
   - `ESPAsyncWebServer`
   - `AsyncTCP`
   - `SPIFFS` or `LittleFS` (based on your board’s support)

3. **Upload the `index.html` file** to SPIFFS using the `ESP32 Sketch Data Upload` tool (install via Arduino IDE extensions or PlatformIO).

4. **Flash the firmware** with the ESP32 sketch (includes server, WebSocket logic, and pin control backend).

5. **Connect to the ESP32's IP address** in a browser on the same WiFi network.

---

## ⚙️ Notes

- Ensure `SPIFFS.begin(true);` is present in your `setup()` to auto-format and mount the file system.
- WebSocket connection uses `/ws` endpoint.
- Real-time oscilloscope may lag if too many analog samples are sent too quickly — tweak sampling rate on the ESP32 if needed.
- Some dropdown selections may refresh unexpectedly — future update will cache states more cleanly.

---

## 📁 File Structure

/ESP32-Web-Debugger/
├── data/
│ └── index.html ← Full HTML/JS frontend UI
├── src/
│ └── main.ino ← ESP32 Arduino code with WebSocket & pin logic
└── README.md

*Coming soon...*

---

## 🧪 Known Bugs

- Dropdown selector for GPIO mode (`INPUT`/`OUTPUT`) resets tile state before user can finish selection
- Occasional flicker on canvas redraw
- PWM input box refreshes too quickly after change — fixing with debounce update system
- UART receive buffer not flushed reliably on certain boards
- EVERTHING

---

## ✨ Future Plans

- Add SPI testing and waveform generation
- Improve debounce visualization
- Upload/download pin config presets
- ESP32-S3 and S2 compatibility checks

---

## 🔗 License

MIT License — Free to use and modify

---
