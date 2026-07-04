# 🚀 HAB Flight Computer Firmware

> Embedded flight computer firmware for a **High Altitude Balloon (HAB)** mission built using **STM32CubeMX**, **STM32 HAL**, and **Arduino**. Designed for reliable environmental sensing, GPS positioning, LoRa telemetry, and modular payload integration.

![STM32](https://img.shields.io/badge/STM32-F303RE-blue?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C-success?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-STM32CubeMX-orange?style=for-the-badge)
![LoRa](https://img.shields.io/badge/Communication-LoRa-green?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)

---

# 📖 Overview

The **HAB Flight Computer** is the onboard embedded system responsible for acquiring, processing, and transmitting telemetry during a High Altitude Balloon (HAB) mission.

Developed using the **STM32 Nucleo-F303RE**, the firmware interfaces with multiple environmental sensors and a GPS receiver, packages telemetry into structured packets, and transmits them to the ground station through an **EBYTE E32 LoRa module**.

The project follows a modular architecture that allows additional sensors and peripherals to be integrated with minimal modifications.

---

# 🎯 Project Objectives

- Acquire environmental data in real time
- Track payload position using GPS
- Transmit telemetry over LoRa
- Provide reliable embedded software for HAB missions
- Maintain a modular and scalable firmware architecture
- Support future payload expansion

---

# 🛰 System Architecture

```
                     +---------------------------+
                     |   STM32 Flight Computer   |
                     |   (NUCLEO-F303RE)         |
                     +------------+--------------+
                                  |
        +-------------------------+-------------------------+
        |                         |                         |
        ▼                         ▼                         ▼
  Environmental Sensors      GPS Receiver            LoRa Module
 (Temp, Pressure, etc.)       (NEO-8M)             (E32-433T30D)
        |                         |                         |
        +-------------------------+-------------------------+
                                  |
                           Telemetry Packet
                                  |
                                  ▼
                          Ground Station Receiver
```

---

# ✨ Features

- STM32CubeMX generated firmware
- STM32 HAL-based peripheral drivers
- Modular firmware architecture
- GPS data acquisition
- LoRa telemetry communication
- I²C sensor integration
- UART communication
- Expandable sensor framework
- Arduino utilities for testing and SD card logging
- Designed for High Altitude Balloon missions

---

# 🔧 Hardware Used

| Hardware | Purpose |
|----------|---------|
| STM32 Nucleo-F303RE | Main Flight Controller |
| EBYTE E32-433T30D | LoRa Telemetry Module |
| NEO-8M GPS | GPS Positioning |
| BMP180  | Pressure & Altitude |
| AHT10 | Temperature & Humidity |
| DS18B20 | External Temperature |
| GUVA-S12SD | UV Sensor |
| Arduino Nano | Testing & SD Card Logging |

---

# 📡 Firmware Responsibilities

The flight computer performs the following tasks:

- Initialize all peripherals
- Read environmental sensors
- Acquire GPS position and UTC time
- Generate telemetry packets
- Transmit packets over LoRa
- Handle sensor communication failures
- Support modular hardware expansion

---

# 📦 Telemetry Parameters

The firmware supports transmission of:

- Packet Number
- Mission Time
- Temperature
- Pressure
- Altitude
- Humidity
- Ultraviolet (UV) Light Intensity
- GPS Latitude
- GPS Longitude
- GPS Time
- GPS Date
- Signal Strength (RSSI)
- Flight Status

---

# 📂 Repository Structure

```
HAB-High-Altitude-Balloon
│
├── Core/
│   ├── Inc/
│   └── Src/
│
├── Drivers/
│
├── Middlewares/
│
├── HAB Mission.ioc
│
├── STM32F303RETX_FLASH.ld
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

# ⚙️ STM32 Peripherals

The firmware makes use of:

- UART
- GPIO
- I²C
- Timers
- DMA (if enabled)
- SPI (future support)
- Interrupts
- HAL Drivers

---

# 🧰 Development Environment

| Software | Version |
|-----------|---------|
| STM32CubeMX | Latest |
| STM32CubeIDE | Latest |
| STM32 HAL | Latest |
| Arduino IDE | 2.x |
| Git | Latest |

---

# 🚀 Getting Started

## Clone the repository

```bash
git clone https://github.com/<YOUR_USERNAME>/HAB-High-Altitude-Balloon.git
```

---

## Open the project

Open

```
HAB Mission.ioc
```

using **STM32CubeMX** or **STM32CubeIDE**.

---

## Generate Code

Generate the project from the IOC file if required.

---

## Build

Compile the firmware inside STM32CubeIDE.

---

## Flash

Upload the firmware using the onboard ST-Link programmer.


# 🛣 Roadmap

- [ ] STM32 SD Card Logging
- [ ] Flight Event Detection
- [ ] Battery Health Monitoring
- [ ] Watchdog Recovery
- [ ] Telemetry Compression
- [ ] CRC Packet Validation
- [ ] Power Optimization
- [ ] Fault-Tolerant Sensor Recovery

---



# 📄 License

This project is licensed under the **MIT License**.

---



# ⭐ Acknowledgements

This project was developed as part of a High Altitude Balloon (HAB) mission to design a reliable onboard flight computer capable of environmental sensing, GPS tracking, and wireless telemetry communication.

The firmware was developed using **STM32CubeMX**, **STM32 HAL**, and **Arduino** following modular embedded software design principles suitable for aerospace and telemetry applications.
