![ESP32](https://img.shields.io/badge/ESP32-Embedded-blue)
![Edge AI](https://img.shields.io/badge/Edge%20AI-TensorFlowLite-green)
![License](https://img.shields.io/badge/License-MIT-yellow)
# ⚡ Edge AI Based SMPS Predictive Failure Detection System

> Intelligent real-time SMPS monitoring and protection using ESP32, Edge AI, and TensorFlow Lite.

---

## 📌 Overview

This project is an **Edge AI-powered SMPS monitoring and predictive failure detection system** developed using the **ESP32 microcontroller** and **Edge Impulse TensorFlow Lite model**.

The system continuously monitors:

- Voltage
- Current
- Temperature
- Power Consumption

Using real-time sensor data and on-device AI inference, the system can detect abnormal operating conditions such as:

- ⚠️ Overvoltage
- ⚠️ Overcurrent
- ⚠️ Overtemperature
- ✅ Normal Operation

When a fault is detected, the ESP32 automatically:
- Disconnects the load using a relay
- Activates buzzer alerts
- Displays fault status on OLED
- Prevents possible SMPS damage

---

# 🚀 Features

- ✅ Real-time SMPS monitoring
- ✅ Edge AI inference on ESP32
- ✅ TensorFlow Lite integration
- ✅ OLED live dashboard
- ✅ Automatic relay protection
- ✅ Audio-visual fault alert system
- ✅ Voltage, current & temperature sensing
- ✅ Compact embedded AI implementation
- ✅ Low-cost and scalable design

---

# 🧠 Technologies Used

- ESP32 WROOM-32
- Edge Impulse
- TensorFlow Lite
- Arduino IDE
- Embedded C++
- SSD1306 OLED
- ACS712 Current Sensor
- DS18B20 Temperature Sensor

---

# 🛠 Hardware Components

| Component | Purpose |
|---|---|
| ESP32 | Main controller |
| ACS712 | Current sensing |
| Voltage Sensor | Voltage monitoring |
| DS18B20 | Temperature monitoring |
| OLED SSD1306 | Live display |
| Relay Module | Load protection |
| Buzzer | Fault indication |
| LM2596 Buck Converter | Voltage regulation |
| SMPS | Power source |

---

# 📊 System Architecture

```text
SMPS → Sensors → ESP32 → Edge AI Inference
                     ↓
         OLED + Relay + Buzzer
