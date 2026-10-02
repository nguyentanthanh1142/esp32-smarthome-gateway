# 🏠 ESP32 Smart Gateway

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Ready-blue.svg)](https://platformio.org/)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![MQTT](https://img.shields.io/badge/Protocol-MQTT-orange.svg)](http://mqtt.org/)
[![Home Assistant](https://img.shields.io/badge/Integration-Home%20Assistant-41BDF5.svg)](https://www.home-assistant.io/)
[![CI/CD](https://github.com/nguyentanthanh1142/esp32-smarthome-gateway/actions/workflows/ci.yml/badge.svg)](https://github.com/nguyentanthanh1142/esp32-smarthome-gateway/actions)

A smart IoT Gateway system designed with a **4-layer architecture (Sensors -> Edge -> Network -> Application)**, achieving **Production-Ready** standards. The system supports fully automated remote firmware updates (MQTT OTA) via GitHub Actions, requiring no physical cable connection after deployment.

---

## 🌟 Key Features

- 🏗️ **Modular Architecture:** Code is clearly separated by function (`hardware`, `sensors`, `mqtt`, `rfid`), making it easy to maintain and scale.
- 🛡️ **Absolute Security:** Sensitive information (MQTT Credentials) is managed via `secret.h` (local) and GitHub Secrets (CI/CD), ensuring it is never exposed in the repository.
- 🔄 **Self-Updating (MQTT OTA):** Automatically downloads and installs new firmware from GitHub Releases when code is pushed to the `main` branch.
- 📶 **Offline-First Architecture:** Core tasks (RFID door unlocking, sensor reading, relay toggling) continue to function normally even when WiFi/MQTT connections are lost.
- ⚙️ **Non-Blocking & Self-Healing:** No `delay()` functions that freeze the system. Features smart WiFi/MQTT auto-reconnection and a Heartbeat mechanism to prevent "zombie sessions".
- 🏠 **Home Assistant Auto-Discovery:** Automatically registers all sensors and devices to Home Assistant without manual YAML configuration.
- 📱 **WiFiManager:** Easily configure new WiFi networks via the `ESP32_Gateway_AP` Access Point without reflashing the code.

---

## 🛠️ Hardware Requirements

| Component | Qty | ESP32 Pins | Notes |
| :--- | :---: | :--- | :--- |
| **ESP32 DevKit V1** | 1 | Main Board | Recommended power supply: 5V - 2A or higher |
| **MFRC522 (RFID)** | 1 | SCK: 18, MISO: 19, MOSI: 23, SDA: 5, RST: 21 | |
| **360° Servo Motor** | 1 | GPIO 32 | Used for door locking/unlocking |
| **Relay Module (2-ch)**| 1 | GPIO 25 (Light), GPIO 27 (Fan) | Active Low |
| **LDR Sensor** | 1 | GPIO 34 (ADC) | Measures light intensity |
| **PIR Motion Sensor** | 1 | GPIO 33 | Detects motion |
| **DHT11 Sensor** | 1 | GPIO 26 | Measures temperature and humidity |
| **KY-026 Flame Sensor**| 1 | GPIO 35 (ADC) | Detects fire/flames |
| **MQ-2 Gas Sensor** | 1 | GPIO 39 (ADC) | Detects smoke/gas leaks |
| **MC-38 Door Sensor** | 1 | GPIO 14 | Detects physical door open/close state |

> ⚠️ **Power Supply Note:** Absolutely do not use low-quality power banks or adapters under 1A. Servos and Relays draw high peak currents, which can cause voltage drops and trigger ESP32 Brownout resets. A **5V - 2A or 3A Adapter** is highly recommended.

---

## 📂 Project Structure

```text
esp32-smarthome-gateway/
├── .github/
│   └── workflows/
│       └── ci.yml                # CI/CD Pipeline: Build, Release & Trigger MQTT OTA
├── include/
│   ├── config.h                  # Hardware configuration, Topics, Intervals
│   ├── globals.h                 # Global variables and objects declaration
│   ├── secret.h.example          # Template file for sensitive info (Committed to Git)
│   └── secret.h                  # Actual secret file (Ignored by Git)
├── src/
│   ├── main.cpp                  # Entry point (setup, loop)
│   ├── hardware.cpp/.h           # Relay, Servo, Timer control
│   ├── sensors.cpp/.h            # Sensor reading logic
│   ├── mqtt_handler.cpp/.h       # MQTT connection, Publish, Subscribe, OTA Handler
│   └── rfid_handler.cpp/.h       # RFID scanning and access control
├── .gitignore                    # Ignore sensitive files and build folders
├── platformio.ini                # PlatformIO Project Configuration
└── README.md                     # This documentation
```

---

## 🚀 Installation & Deployment

### 1. Local Setup (For Developers)
1. Clone the repository:
   ```bash
   git clone https://github.com/nguyentanthanh1142/esp32-smarthome-gateway.git
   cd esp32-smarthome-gateway
   ```
2. Create the secret configuration file from the template:
   - **Windows:** `copy include\secret.h.example include\secret.h`
   - **macOS/Linux:** `cp include/secret.h.example include/secret.h`
3. Open `include/secret.h` and fill in your actual MQTT credentials.
4. Open the project in **VS Code + PlatformIO Extension**.
5. Click **Build (✔)** and **Upload (→)** via USB cable.

### 2. Automated CI/CD & OTA Setup (Production)
To enable automatic firmware updates every time you `git push`, follow these steps:

1. **Ensure Repository is Public:** GitHub requires the repo to be Public so the ESP32 can download the `.bin` file from Releases without complex token authentication.
2. **Configure GitHub Secrets:**
   - Go to your GitHub repository ➔ **Settings** ➔ **Secrets and variables** ➔ **Actions**.
   - Add the following 3 secrets:
     - `MQTT_SERVER`: (e.g., `your-mqtt-server.com` or your IP)
     - `MQTT_USER`: Your MQTT username
     - `MQTT_PASS`: Your MQTT password
3. **Activation:** Whenever you modify code and run `git push origin main`, GitHub Actions will automatically:
   - Build the firmware.
   - Create a new GitHub Release and attach the `firmware.bin` file.
   - Send an MQTT command to the ESP32 to download and install the update.

---

## 🏠 Home Assistant Integration

The system uses **MQTT Auto-Discovery**. You only need to:
1. Ensure an MQTT Broker (e.g., Mosquitto) is configured in Home Assistant.
2. Once the ESP32 connects successfully, it will automatically send the configuration for all sensors and devices.
3. Go to **Settings** ➔ **Devices & Services** in HA, and you will see the **ESP32 Gateway 02** device appear with all its entities (Light, Fan, Door Lock, Temperature, Gas, etc.).

---

## 🔧 Troubleshooting

| Issue | Possible Cause | Solution |
| :--- | :--- | :--- |
| **OTA fails (-104): Wrong HTTP Code** | Repository is Private, or Release URL is incorrect. | Change repo to **Public**. Verify the download URL in an Incognito browser window. |
| **HA shows "Unavailable" but Serial runs** | MQTT connection lost due to "Zombie Session" or weak power supply. | Check power adapter (needs ≥ 2A). The code already includes Heartbeat and auto-reconnect mechanisms. |
| **Cannot find WiFi `ESP32_Gateway_AP`** | ESP32 has already saved a WiFi network and is connected successfully. | Check Serial Monitor for the current IP address, or press the EN button to reset. |
| **Compilation error: `secret.h: No such file`** | The `secret.h` file has not been created or was deleted. | Run the copy command from `secret.h.example` as guided in the Local Setup section. |

---

## 🤝 Contributing

Any contributions, bug reports (Issues), or feature proposals (Pull Requests) are highly welcome! Let's make this project even better.

---
*Developed with ❤️ by [Nguyễn Tấn Thành](https://github.com/nguyentanthanh1142)*