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
- Local display with 0.96" OLED SSD1306
- Wi-Fi connectivity via ESP32
- Low-stock and usage alerts
- Cloud integration (ThingSpeak / Blynk ready)
- Affordable and scalable for household use

## 🛠 Hardware Components

- **Microcontroller:** ESP32 DevKitC V4
- **Weight Sensor:** HX711 + Load Cell (50kg)
- **RFID:** MFRC522 Module
- **Display:** SSD1306 OLED 0.96"

## 📁 Project Structure

```
.
├── README.md
├── hardware/
│   └── wokwi-diagram.json          # Wokwi Simulation
├── src/
│   └── main.ino                    # Main Arduino Sketch
├── docs/
│   ├── Final_Report.pdf
│   └── Presentation.pptx
├── images/
│   └── prototype.jpg
└── LICENSE
```

## 🚀 How to Use

1. Open the Wokwi diagram in [Wokwi](https://wokwi.com)
2. Upload the code from `src/main.ino` to ESP32
3. Configure Wi-Fi credentials
4. Monitor data on Serial or Cloud platform

## 📊 Results

- Accurate weight measurement (±5g after calibration)
- Real-time cloud updates
- Effective low-stock alerts

## 📄 Documentation

- Full project report in `docs/`
- Presentation slides in `docs/`

## License

This project is open-sourced under the MIT License.

---

**Made with ❤️ for better household management**
