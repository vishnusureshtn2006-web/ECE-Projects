# 🚪 Smart Door Access System

A dual-biometric access control and security system powered by an **ESP32**. It features two-factor authentication combining **RFID** and **Fingerprint verification**, a local **Wi-Fi Web Dashboard** for real-time monitoring and enrollment, and automated servo lock control.

---

## 🛠️ Features

* **Two-Factor Authentication (2FA):** Requires both a valid RFID tag scan and fingerprint match within a 10-second timeout window to unlock.
* **On-Board Web Dashboard:** Hosts an asynchronous web page served directly from the ESP32 via its soft Access Point (AP).
* **Live Status Updates:** Uses JSON polling to display live system, door, RFID, and fingerprint sensor states on the web UI.
* **Web Enrollment:** Easily trigger RFID and Fingerprint registration modes directly through dashboard buttons.
* **Non-Blocking Architecture:** Keeps the local web server active and responsive while handling sensor verification timeouts.

---

## 🧱 Hardware Components

| Component | Model / Specs | Function |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 DevKit V1 | Main processor & SoftAP host |
| **RFID Reader** | MFRC522 (13.56 MHz SPI) | Primary access verification |
| **Fingerprint Sensor** | AS608 / R307 (UART) | Secondary biometric verification |
| **Actuator** | Micro Servo (e.g., SG90 / MG90S) | Door latch mechanism |
| **Power Supply** | 5V / 2A | Powers ESP32 & Servo motor |

---

## 📌 Pinout & Wiring

### MFRC522 RFID (SPI)
* **SS / SDA:** GPIO 21
* **RST:** GPIO 22
* **SCK:** GPIO 18
* **MISO:** GPIO 19
* **MOSI:** GPIO 23

### AS608 Fingerprint Sensor (UART2)
* **TX (Sensor):** GPIO 16 (RX2)
* **RX (Sensor):** GPIO 17 (TX2)

### Servo Motor
* **Signal:** GPIO 25

---

## ⚡ Quick Start

1. **Libraries Required:** Install the following via Arduino Library Manager:
   * `MFRC522` by Miguel Balboa
   * `Adafruit Fingerprint Sensor Library` by Adafruit
   * `ESP32Servo` by Kevin Harrington / John K. Bennett
2. **Flash Firmware:** Open `smartlock.ino` in Arduino IDE, select **ESP32 Dev Module**, and upload.
3. **Connect to Wi-Fi:**
   * **SSID:** `SmartDoor`
   * **Password:** `12345678`
4. **Access Dashboard:** Open your browser and go to `http://192.168.4.1`.
