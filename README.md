# 🚀 HAB Flight Computer Firmware

> Embedded flight computer firmware for a **High Altitude Balloon (HAB)** mission built using **Arduino Nano** and the **Arduino IDE**. Designed for reliable environmental sensing, GPS positioning, SD card data logging, LoRa telemetry, and modular payload integration.

![Arduino Nano](https://img.shields.io/badge/Arduino-Nano-blue?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-success?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-Arduino%20IDE-orange?style=for-the-badge)
![LoRa](https://img.shields.io/badge/Communication-LoRa-green?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)

---

# 📖 Overview

The **HAB Flight Computer** is the onboard embedded system responsible for acquiring, processing, logging, and transmitting telemetry during a High Altitude Balloon (HAB) mission.

The system is built around an **Arduino Nano**, which interfaces with multiple environmental sensors, a GPS receiver, an SD card module, and an **EBYTE E32-433T30D LoRa module**.

The firmware continuously collects sensor data, performs onboard calculations such as altitude estimation, climb-rate calculation, temperature gradient, sensor health monitoring, and flight-phase detection, while simultaneously storing full-resolution telemetry data on an SD card and transmitting compact telemetry packets through LoRa.

The architecture is designed to remain modular, allowing additional sensors and payload peripherals to be integrated with minimal modifications.

---

# 🎯 Project Objectives

* Acquire environmental data in real time
* Track payload position using GPS
* Estimate altitude using pressure and GPS data
* Monitor acceleration and angular motion
* Monitor battery voltage
* Log telemetry locally to an SD card
* Transmit telemetry over LoRa
* Detect different flight phases
* Monitor sensor health
* Provide reliable embedded software for HAB missions
* Maintain a modular and scalable firmware architecture
* Support future payload expansion

---

# 🛰 System Architecture

```text
                     +---------------------------+
                     |     Arduino Nano          |
                     |     Flight Computer       |
                     +------------+--------------+
                                  |
       +--------------------------+--------------------------+
       |             |            |            |             |
       ▼             ▼            ▼            ▼             ▼
 Environmental      GPS         MPU6050      SD Card       Battery
    Sensors       NEO-8M      Accel/Gyro     Logging       Monitor
       |             |            |            |             |
       +-------------+------------+------------+-------------+
                                  |
                                  ▼
                         Telemetry Processing
                                  |
                     +------------+-------------+
                     |                          |
                     ▼                          ▼
                SD Card Log               E32-433T30D
                HABLOG.CSV                    LoRa
                                                 |
                                                 ▼
                                      Ground Station Receiver
```

---

# ✨ Features

* Arduino Nano-based flight computer
* Real-time environmental sensing
* GPS data acquisition using NMEA
* LoRa telemetry communication
* Local SD card telemetry logging
* I²C sensor integration
* SoftwareSerial GPS communication
* Hardware UART LoRa communication
* Battery voltage monitoring
* Flight-phase detection
* Altitude source selection
* Sensor health monitoring
* GPS fix freshness monitoring
* Altitude sanity checking
* Climb-rate calculation
* Temperature difference and gradient calculation
* Compact binary LoRa telemetry packets
* XOR checksum validation
* Modular sensor architecture
* Designed for High Altitude Balloon missions

---

# 🔧 Hardware Used

| Hardware                | Purpose                          |
| ----------------------- | -------------------------------- |
| Arduino Nano            | Main Flight Computer             |
| EBYTE E32-433T30D       | LoRa Telemetry Module            |
| NEO-8M GPS              | GPS Positioning                  |
| BMP180                  | Pressure, Temperature & Altitude |
| AHT10                   | Temperature & Humidity           |
| MPU6050                 | 3-Axis Accelerometer & Gyroscope |
| DS18B20 ×2              | Internal & External Temperature  |
| GUVA-S12SD              | UV Sensor                        |
| MicroSD Card Module     | Local Telemetry Logging          |
| Battery Voltage Divider | Battery Monitoring               |

---

# 📡 Firmware Responsibilities

The flight computer performs the following tasks:

* Initialize all sensors and peripherals
* Read pressure and temperature from BMP180
* Read temperature and humidity from AHT10
* Read acceleration and gyroscope data from MPU6050
* Read internal and external temperatures using DS18B20 sensors
* Measure UV intensity using the analog UV sensor
* Monitor battery voltage
* Acquire GPS position, altitude, speed, and satellite information
* Calculate altitude and climb rate
* Calculate temperature difference and temperature gradient
* Determine the current flight phase
* Monitor sensor health
* Log full-resolution telemetry to the SD card
* Generate compact binary telemetry packets
* Calculate telemetry packet checksum
* Transmit telemetry over the E32 LoRa module

---

# 📦 Telemetry Parameters

The firmware supports logging and transmission of:

* Mission Time
* BMP180 Temperature
* Atmospheric Pressure
* Pressure Altitude
* Altitude Validity
* Altitude Source
* MPU6050 X/Y/Z Acceleration
* Acceleration Magnitude
* MPU6050 X/Y/Z Gyroscope
* AHT10 Temperature
* Relative Humidity
* Dew Point
* Internal Temperature
* External Temperature
* Temperature Difference
* Temperature Gradient
* UV Index
* Battery Voltage
* Current Flight Phase
* Maximum Altitude
* Best Altitude
* GPS Latitude
* GPS Longitude
* GPS Altitude
* GPS Speed
* GPS Satellite Count
* GPS Fix Status
* Sensor Health Status
* LoRa Transmission Status
* LoRa Transmission Skip Count

The telemetry packet uses a compact binary structure with synchronization bytes and an XOR checksum for basic packet integrity verification.

---

# 📂 Repository Structure

```text
HAB-High-Altitude-Balloon
│
├── HAB_Flight_Computer/
│   └── HAB_Flight_Computer.ino
│
├── Arduino/
│   ├── SD_Logger/
│   ├── Receiver_Test/
│   └── LoRa_Test/
│
├── Documentation/
│   ├── Images/
│   ├── Wiring_Diagram.png
│   ├── BlockDiagram.png
│   └── Pinout.png
│
├── Hardware/
│
├── LICENSE
│
└── README.md
```

---

# ⚙️ Arduino Nano Interfaces

The firmware makes use of:

* I²C
* SPI
* Hardware UART
* SoftwareSerial
* GPIO
* Analog ADC
* OneWire
* Digital inputs
* Digital outputs
* SD card interface

### Pin Configuration

| Nano Pin | Function           |
| -------- | ------------------ |
| D0 / RX  | E32 LoRa RX        |
| D1 / TX  | E32 LoRa TX        |
| D2       | GPS RX             |
| D3       | GPS TX             |
| D4       | Internal DS18B20   |
| D5       | External DS18B20   |
| D6       | E32 AUX            |
| D7       | LoRa TX Status LED |
| D10      | SD Card CS         |
| A0       | UV Sensor          |
| A1       | Battery Voltage    |
| A4 / A5  | I²C SDA / SCL      |

The current firmware defines these pins directly in the source code, including D2/D3 for GPS, D4/D5 for the two DS18B20 sensors, D6 for E32 AUX, D10 for SD card chip select, A0 for UV sensing, and A1 for battery measurement.

---

# 🧰 Development Environment

| Software         | Version                    |
| ---------------- | -------------------------- |
| Arduino IDE      | 2.x                        |
| Arduino AVR Core | Compatible Nano/ATmega328P |
| Git              | Latest                     |

### Arduino Libraries

The firmware uses:

* Wire
* SPI
* SD
* OneWire
* SoftwareSerial
* Math

These libraries are included at the beginning of the flight computer firmware.

---

# 🚀 Getting Started

## Clone the repository

```bash
git clone https://github.com/<YOUR_USERNAME>/HAB-High-Altitude-Balloon.git
```

---

## Open the project

Open the Arduino sketch:

```text
HAB_Flight_Computer.ino
```

using **Arduino IDE**.

---

## Select Board

In Arduino IDE, select:

```text
Board: Arduino Nano
Processor: ATmega328P
```

Select the appropriate COM port for the connected Arduino Nano.

---

## Install Libraries

Install the required libraries through the Arduino IDE Library Manager if they are not already available:

```text
Wire
SPI
SD
OneWire
SoftwareSerial
```

---

## Build

Compile the firmware using the Arduino IDE.

Before uploading, verify:

* Sensor wiring
* I²C addresses
* SD card wiring
* GPS wiring
* LoRa wiring
* Battery voltage divider values
* E32 AUX connection
* Correct Arduino Nano processor selection

---

## Upload

Connect the Arduino Nano through USB and upload the firmware using the Arduino IDE.

---

# 📝 Data Logging

The flight computer stores telemetry locally on a microSD card.

The primary log file is:

```text
HABLOG.CSV
```

The SD logging system records full-resolution sensor and mission data, including environmental parameters, GPS information, flight phase, altitude information, sensor health, and LoRa transmission status.

Example data categories include:

```text
millis
bmp_tempC
bmp_pressPa
bmp_altitude_m
mpu_ax
mpu_ay
mpu_az
mpu_gx
mpu_gy
mpu_gz
aht_tempC
aht_humPct
dew_point_C
ds_in_tempC
ds_out_tempC
temp_diff_C
temp_gradient_Cps
uv_index
battery_voltage
flight_phase
max_altitude_m
best_altitude_m
gps_lat
gps_lon
gps_alt_m
gps_speed_kmh
gps_satellites
gps_fix_valid
sensor_health_hex
tx_success
tx_skip_total
```

---

# 📡 LoRa Telemetry

The **EBYTE E32-433T30D** is used as the primary wireless telemetry link.

The Nano communicates with the E32 module through its hardware UART:

```text
Arduino Nano D1/TX → E32 RX
Arduino Nano D0/RX ← E32 TX
```

The E32 AUX pin is monitored by the Nano to determine whether the module is ready for transmission.

Before transmitting, the firmware waits for the E32 module to become ready. If the module remains busy beyond the configured timeout, the packet is skipped and the event is recorded.

---

# 🛰 Flight Phase Detection

The firmware automatically determines the current flight phase using altitude and climb-rate information.

Supported flight phases:

```text
PRELAUNCH
ASCENT
NEAR_APOGEE
DESCENT
LANDED
```

The flight phase is determined using configurable climb-rate, altitude-window, and landing thresholds.

---

# 🧠 Onboard Data Processing

The firmware performs several calculations before logging and transmission:

### Altitude Selection

The system can use:

```text
BMP180 Altitude
        ↓
   Validity Check
        ↓
   GPS Altitude
        ↓
 Best Available Altitude
```

BMP180 altitude readings are checked for physically implausible changes before being trusted.

### Climb Rate

```text
Climb Rate = ΔAltitude / ΔTime
```

### Temperature Difference

```text
Temperature Difference =
Internal Temperature - External Temperature
```

### Temperature Gradient

```text
Temperature Gradient =
ΔExternal Temperature / ΔTime
```

### Acceleration Magnitude

```text
Acceleration Magnitude =
√(Ax² + Ay² + Az²)
```

---

# ❤️ Sensor Health Monitoring

A sensor health bitmask is maintained by the firmware.

The system monitors:

* BMP180
* MPU6050
* AHT10
* Internal DS18B20
* External DS18B20
* BMP180 altitude validity
* GPS fix validity
* SD logging status

This allows the ground station to determine which sensors are functioning correctly during the mission.

---

# 🛡 Reliability Features

The firmware includes several mechanisms intended to improve mission reliability:

* Sensor validity flags
* GPS fix freshness checking
* BMP180 altitude sanity checking
* Sensor health bitmask
* LoRa AUX monitoring
* LoRa transmission timeout
* Telemetry checksum
* SD card logging
* Flight-phase state machine
* GPS NMEA buffer overflow protection
* Automatic fallback from BMP180 altitude to GPS altitude
* Tracking of successful and skipped LoRa transmissions

---

# 🛣 Roadmap

* [ ] Watchdog Recovery
* [ ] Flight Event Detection Improvements
* [ ] Advanced Battery Health Monitoring
* [ ] Telemetry Compression
* [ ] Enhanced CRC Packet Validation
* [ ] Power Optimization
* [ ] Fault-Tolerant Sensor Recovery
* [ ] Ground Station Telemetry Dashboard
* [ ] Automated Post-Flight Data Analysis
* [ ] Additional Flight Sensors

---

# 📄 License

This project is licensed under the **MIT License**.

---

# ⭐ Acknowledgements

This project was developed as part of a High Altitude Balloon (HAB) mission to design a reliable onboard flight computer capable of environmental sensing, GPS tracking, local data logging, and wireless telemetry communication.

The flight computer is based on the **Arduino Nano** and uses multiple environmental sensors, an **EBYTE E32-433T30D LoRa module**, GPS receiver, and microSD storage to provide an integrated telemetry and data-logging platform suitable for HAB missions.
