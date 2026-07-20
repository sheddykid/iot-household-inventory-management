# IoT Household Inventory Management System

![Project Banner](https://via.placeholder.com/800x200/0066cc/ffffff?text=IoT+Inventory+Management)

## Overview

A smart IoT-based system for real-time household inventory monitoring using weight sensors, RFID for item identification, and cloud connectivity. Developed as a Final Year Project at Redeemer's University.

**Student:** Babalola Emmanuel Timilehin  
**Matric No:** RUN/CPE/20/8934  
**Supervisor:** Engineer O. Olayinka

## ✨ Features

- Real-time weight monitoring using HX711 Load Cell
- Item identification with MFRC522 RFID
- Local display with 16x4 LCD
- Wi-Fi connectivity via ESP32-C6
- Low-stock and usage alerts
- Cloud integration (ThingSpeak ready)
- Affordable and scalable for household use

## 🛠 Hardware Components

- **Microcontroller:** ESP32-C6 DevKitC V4
- **Weight Sensor:** HX711 + 4 Load CellS (50kg)
- **RFID:** MFRC522 Module
- **Display:**  16x4 LCD

## 📁 Project Structure

```
.
├── sketch.ino                    # Main ESP32 firmware — RFID + HX711 + LCD + WiFi + ThingSpeak + email alerts
├── platformio.ini                # PlatformIO build config
├── wokwi.toml                    # Wokwi simulator config (VS Code extension)
├── diagram.json                  # Wokwi circuit diagram — 16x4 LCD variant
├── hardware/
│   └── wokwi-diagram.json        # Wokwi circuit diagram — OLED variant (diverges from diagram.json above; not kept in sync)
├── RFID_calibration              # Standalone test sketch — RFID read/calibration only
├── RFID_ESP_16x4_test            # Standalone test sketch — RFID + LCD integration
├── Test_only_LCD                 # Standalone test sketch — LCD only
├── hardware_diagonstic           # Standalone diagnostic sketch — tests I2C/RFID/HX711/WiFi independently
├── email_alert_app_script        # Google Apps Script (doPost) — sends low-stock email alerts
├── index.html                    # Web dashboard (Chart.js, fetches from ThingSpeak) — live at the link below
├── Inventory_Email_Preview.html  # Static preview of the alert email's HTML layout
├── supabase-config.js            # Supabase URL/anon key — not currently referenced by index.html
├── favicon.ico
├── _headers                      # Security header rules (Netlify/Cloudflare Pages format — not applied by GitHub Pages)
├── Final_Report.pdf
├── LICENSE
└── README.md
```

> **Note:** the standalone test sketches (`RFID_calibration`, `RFID_ESP_16x4_test`, `Test_only_LCD`, `hardware_diagonstic`) and `email_alert_app_script` have no file extension. Rename to `.ino` before opening the test sketches in Arduino IDE, or open them in PlatformIO/any text editor as-is.

## 🚀 How to Use

**1. Wire the hardware**
Follow `diagram.json` (16x4 LCD build) — this is the circuit `sketch.ino` is written against. `hardware/wokwi-diagram.json` describes a different OLED-based variant and is not what the current firmware expects.

**2. Configure and flash the firmware**
- Open the project in PlatformIO (`platformio.ini` targets `esp32doit-devkit-v1`) or Arduino IDE.
- Install the libraries listed in `platformio.ini`: `LiquidCrystal_I2C`, `MFRC522`, `ThingSpeak`, `HX711`.
- In `sketch.ino`, set your own values for `ssid`, `password`, `channelID`, `writeAPIKey`, and `googleScriptURL`.
- Upload to the board.

**3. Set up cloud logging**
- Create a ThingSpeak channel with 8 fields (weight + stock % for each of the 4 items).
- Put your channel ID and Write API Key into `sketch.ino`.

**4. Set up email alerts**
- Create a new Google Apps Script project and paste in the contents of `email_alert_app_script`.
- Deploy it as a Web App and put the deployment URL into `sketch.ino`'s `googleScriptURL`.
- Edit the recipient list inside the script itself.

**5. Test subsystems before running the full firmware**
Flash `RFID_calibration`, `Test_only_LCD`, or `hardware_diagonstic` individually first to confirm each sensor works in isolation — this catches wiring issues before debugging them inside the full `sketch.ino`.

**6. View the dashboard**
Live at [sheddykid.github.io/iot-household-inventory-management](https://sheddykid.github.io/iot-household-inventory-management/), or open `index.html` locally. Select a date range and click Fetch Data to pull from ThingSpeak.

## 📊 Results

- Accurate weight measurement (±5g after calibration)
- Real-time cloud updates
- Effective low-stock alerts

## 📄 Documentation

- Full project report in [Report](https://github.com/sheddykid/iot-household-inventory-management/blob/main/Final_Project.pdf)
- Presentation slides in `docs/`

## License

This project is open-sourced under the MIT License.

---

**Made for better household management**
