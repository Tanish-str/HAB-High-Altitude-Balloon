# 🚀 HAB01 — Flight Computer & Hardware

> Onboard flight computer and telemetry hardware for the **HAB01 High Altitude Balloon mission**. The system acquires environmental, motion, GPS, and power data, stores full-resolution telemetry locally, and transmits compact telemetry packets to the ground station using LoRa.

---

## 📖 Overview

The **HAB01 Flight Computer** is the onboard embedded system responsible for collecting, processing, logging, and transmitting telemetry throughout the High Altitude Balloon mission.

The system is built around an **Arduino Nano** and integrates:

* Environmental sensors
* GPS receiver
* Accelerometer and gyroscope
* Internal and external temperature sensors
* UV sensor
* Battery voltage monitoring
* MicroSD card storage
* EBYTE E32-433T30D LoRa telemetry

The flight computer performs onboard calculations such as:

* Pressure altitude estimation
* GPS altitude tracking
* Best-available altitude selection
* Climb-rate calculation
* Temperature difference
* Temperature gradient
* Acceleration magnitude
* Sensor health monitoring
* GPS fix monitoring
* Flight-phase detection

Telemetry is simultaneously:

1. Logged locally to the onboard microSD card
2. Encoded into compact binary packets
3. Transmitted to the ground station through LoRa

The architecture is designed to remain modular so additional sensors and payload systems can be integrated in future mission iterations.

---

# 🎯 Mission Objectives

The flight computer is designed to:

* Acquire environmental data in real time
* Track payload position using GPS
* Estimate and monitor altitude
* Monitor acceleration and angular motion
* Monitor battery voltage
* Log telemetry locally
* Transmit telemetry over LoRa
* Detect flight phases
* Monitor sensor health
* Detect invalid or stale sensor readings
* Provide reliable onboard data during flight
* Maintain a modular hardware architecture
* Support future payload expansion

---

# 🛰️ Hardware Architecture

```text
                         ┌──────────────────────────┐
                         │      Arduino Nano        │
                         │      Flight Computer     │
                         └────────────┬─────────────┘
                                      │
          ┌───────────────┬───────────┼───────────┬───────────────┐
          │               │           │           │               │
          ▼               ▼           ▼           ▼               ▼
     BMP180 / AHT10     NEO-8M      MPU6050    DS18B20 ×2      GUVA-S12SD
     Environmental       GPS       Accel/Gyro   Temperature       UV
       Sensors
          │               │           │           │               │
          └───────────────┴───────────┼───────────┴───────────────┘
                                      │
                                      ▼
                             Telemetry Processing
                                      │
                         ┌────────────┴────────────┐
                         │                         │
                         ▼                         ▼
                   MicroSD Card             E32-433T30D
                   HABLOG.CSV                   LoRa
                                                 │
                                                 │ RF
                                                 ▼
                                      Ground Station Receiver
                                                 │
                                                 ▼
                                          Serial / Dashboard
```

---

# 🔧 Hardware Components

| Component                   | Purpose                                        |
| --------------------------- | ---------------------------------------------- |
| **Arduino Nano**            | Main flight computer                           |
| **EBYTE E32-433T30D**       | Long-range LoRa telemetry                      |
| **NEO-8M GPS**              | Position, altitude, speed and satellite data   |
| **BMP180**                  | Atmospheric pressure, temperature and altitude |
| **AHT10**                   | Temperature and relative humidity              |
| **MPU6050**                 | 3-axis acceleration and gyroscope              |
| **DS18B20 ×2**              | Internal and external temperature              |
| **GUVA-S12SD**              | UV sensing                                     |
| **MicroSD Card Module**     | Local telemetry storage                        |
| **Battery Voltage Divider** | Battery voltage measurement                    |

---

# 📡 Communication Architecture

The HAB01 telemetry system uses the following communication path:

```text
┌─────────────────────┐
│   Flight Computer   │
│    Arduino Nano     │
└──────────┬──────────┘
           │
           │ Binary Telemetry
           ▼
┌─────────────────────┐
│  E32-433T30D LoRa   │
│      Transmitter    │
└──────────┬──────────┘
           │
           │ 433 MHz RF Link
           ▼
┌─────────────────────┐
│   Ground Station    │
│    Arduino Uno      │
└──────────┬──────────┘
           │
           │ USB Serial / CSV
           ▼
┌─────────────────────┐
│    Serial Bridge    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Backend + Database  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Live HAB Dashboard  │
└─────────────────────┘
```

The onboard flight computer is therefore the **source of truth for flight telemetry**, while the ground station receives and forwards that telemetry to the software system.

---

# 🔌 Arduino Nano Pin Configuration

| Nano Pin    | Function              |
| ----------- | --------------------- |
| **D0 / RX** | E32 LoRa TX → Nano RX |
| **D1 / TX** | Nano TX → E32 LoRa RX |
| **D2**      | GPS RX                |
| **D3**      | GPS TX                |
| **D4**      | Internal DS18B20      |
| **D5**      | External DS18B20      |
| **D6**      | E32 AUX               |
| **D7**      | LoRa TX Status LED    |
| **D10**     | MicroSD Card CS       |
| **A0**      | GUVA-S12SD UV Sensor  |
| **A1**      | Battery Voltage       |
| **A4**      | I²C SDA               |
| **A5**      | I²C SCL               |

### Interfaces Used

The flight computer uses:

* I²C
* SPI
* Hardware UART
* SoftwareSerial
* GPIO
* Analog ADC
* OneWire
* Digital inputs
* Digital outputs
* MicroSD interface

---

# 🌡️ Environmental Sensors

## BMP180

The BMP180 provides:

* Atmospheric pressure
* Temperature
* Pressure-derived altitude

The firmware performs validity and sanity checks before using BMP180 altitude as the primary altitude source.

---

## AHT10

The AHT10 provides:

* Temperature
* Relative humidity

The firmware additionally calculates dew point from the measured temperature and humidity.

---

## DS18B20 ×2

Two DS18B20 sensors are used for thermal monitoring:

```text
DS18B20 #1 → Internal / Payload Temperature

DS18B20 #2 → External / Ambient Temperature
```

The firmware uses these readings to calculate:

```text
Temperature Difference
        =
Internal Temperature - External Temperature
```

and:

```text
Temperature Gradient
        =
ΔExternal Temperature / ΔTime
```

This allows thermal behavior of the payload to be observed during ascent and descent.

---

## GUVA-S12SD

The GUVA-S12SD provides an analog measurement associated with UV intensity.

The sensor is connected to the Arduino Nano analog input:

```text
GUVA-S12SD → A0
```

The resulting value is incorporated into the telemetry stream as the UV index/value used by the firmware.

---

# 🧭 Motion Sensing

## MPU6050

The MPU6050 provides:

* X-axis acceleration
* Y-axis acceleration
* Z-axis acceleration
* X-axis gyroscope
* Y-axis gyroscope
* Z-axis gyroscope

The firmware additionally calculates acceleration magnitude:

```text
Acceleration Magnitude
=
√(Ax² + Ay² + Az²)
```

Acceleration data can be used to identify events such as:

* Launch
* Balloon burst
* Sudden payload movement
* Parachute deployment
* Landing impact

---

# 🛰️ GPS System

The **NEO-8M GPS receiver** provides:

* Latitude
* Longitude
* GPS altitude
* Ground speed
* Satellite count
* Fix validity

The GPS communicates with the Arduino Nano using SoftwareSerial.

```text
GPS RX → Nano D2
GPS TX → Nano D3
```

The firmware also tracks GPS fix freshness so stale position data is not blindly treated as current telemetry.

---

# 📡 LoRa Telemetry

The **EBYTE E32-433T30D** is used as the primary wireless telemetry link.

### Connections

```text
Arduino Nano D1 / TX ─────► E32 RX

Arduino Nano D0 / RX ◄───── E32 TX

Arduino Nano D6 ◄────────── E32 AUX
```

The E32 AUX line is monitored by the firmware to determine whether the LoRa module is ready.

Before transmission, the firmware waits for the module to become available.

If the module remains busy beyond the configured timeout:

```text
Telemetry packet
      ↓
E32 busy?
      ↓
   YES
      ↓
Timeout
      ↓
Packet skipped
      ↓
Skip counter updated
```

Successful and skipped transmissions are tracked by the firmware.

---

# 📦 Telemetry Packet

The LoRa telemetry uses a **compact binary packet format** rather than transmitting the complete CSV record over the RF link.

The packet contains synchronization information and an XOR checksum for basic integrity validation.

```text
┌──────────────┐
│ Sync Bytes   │
├──────────────┤
│ Telemetry    │
│ Payload      │
├──────────────┤
│ XOR Checksum │
└──────────────┘
```

The ground station validates the packet before decoding it.

This allows corrupted packets to be rejected instead of being passed into the telemetry database/dashboard.

---

# 📊 Telemetry Parameters

The flight computer supports telemetry including:

### Mission

* Mission time
* Current flight phase
* Maximum altitude
* Best altitude

### Atmospheric

* BMP180 temperature
* Atmospheric pressure
* Pressure altitude
* Altitude validity
* Altitude source

### Motion

* X acceleration
* Y acceleration
* Z acceleration
* Acceleration magnitude
* X gyroscope
* Y gyroscope
* Z gyroscope

### Temperature & Humidity

* AHT10 temperature
* Relative humidity
* Dew point
* Internal temperature
* External temperature
* Temperature difference
* Temperature gradient

### UV & Power

* UV index
* Battery voltage

### GPS

* Latitude
* Longitude
* GPS altitude
* GPS speed
* Satellite count
* GPS fix status

### System Health

* Sensor health status
* LoRa transmission status
* LoRa transmission skip count
* SD logging status

---

# 🧠 Onboard Data Processing

The flight computer does more than simply read sensors. Several values are processed onboard before they are logged and transmitted.

---

## Altitude Source Selection

The system can evaluate both pressure altitude and GPS altitude.

```text
          BMP180 Altitude
                 │
                 ▼
          Validity Check
                 │
          ┌──────┴──────┐
          │             │
        Valid         Invalid
          │             │
          ▼             ▼
    Use BMP180       GPS Altitude
          │             │
          └──────┬──────┘
                 ▼
          Best Altitude
```

BMP180 altitude is subjected to sanity checking before it is trusted.

---

## Climb Rate

The firmware calculates climb rate using:

```text
Climb Rate = ΔAltitude / ΔTime
```

This value contributes to flight-phase detection and provides useful information about the balloon's ascent and descent.

---

## Temperature Difference

```text
Temperature Difference
=
Internal Temperature - External Temperature
```

This provides a simple measure of the thermal environment around the payload.

---

## Temperature Gradient

```text
Temperature Gradient
=
ΔExternal Temperature / ΔTime
```

This helps identify how rapidly the external temperature is changing throughout the flight.

---

## Acceleration Magnitude

```text
Acceleration Magnitude
=
√(Ax² + Ay² + Az²)
```

This produces a single value representing the overall acceleration magnitude.

---

# 🛰️ Flight Phase Detection

The flight computer automatically determines the current mission phase.

Supported states:

```text
PRELAUNCH
    ↓
ASCENT
    ↓
NEAR_APOGEE
    ↓
DESCENT
    ↓
LANDED
```

Flight-phase detection uses altitude and climb-rate information together with configurable thresholds.

The current phase is included in the telemetry transmitted to the ground station.

---

# ❤️ Sensor Health Monitoring

The firmware maintains a sensor-health bitmask.

The monitored systems include:

| System           | Health Status          |
| ---------------- | ---------------------- |
| BMP180           | Monitored              |
| MPU6050          | Monitored              |
| AHT10            | Monitored              |
| Internal DS18B20 | Monitored              |
| External DS18B20 | Monitored              |
| BMP180 altitude  | Validity monitored     |
| GPS              | Fix validity monitored |
| SD card          | Write status monitored |

This information allows the ground station and dashboard to determine whether individual sensors are operating correctly.

---

# 💾 SD Card Data Logging

The flight computer maintains a local copy of the mission telemetry on a microSD card.

Primary log:

```text
HABLOG.CSV
```

The onboard SD log is intended to provide a **full-resolution backup of the mission data**, independent of the LoRa link.

This is particularly important because wireless telemetry may experience:

* Packet loss
* RF interference
* Antenna orientation issues
* Temporary signal blockage
* Ground-station connection problems

The onboard SD card therefore acts as the primary post-flight source for complete telemetry analysis.

---

# 📄 Example Telemetry Fields

The onboard CSV log contains fields such as:

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

# 🛡️ Reliability Features

The flight computer incorporates several mechanisms intended to improve mission reliability:

* Sensor validity flags
* GPS fix freshness monitoring
* BMP180 altitude sanity checking
* Sensor health bitmask
* LoRa AUX monitoring
* LoRa transmission timeout
* XOR telemetry checksum
* Local SD card logging
* Flight-phase state machine
* GPS buffer protection
* BMP180 → GPS altitude fallback
* LoRa transmission success tracking
* LoRa transmission skip tracking

The system is designed so that a temporary problem with one subsystem does not automatically invalidate the rest of the telemetry system.

---

# 🧰 Development Environment

| Software             | Configuration |
| -------------------- | ------------- |
| Arduino IDE          | 2.x           |
| Arduino Board        | Arduino Nano  |
| MCU                  | ATmega328P    |
| Programming Language | C/C++         |
| Version Control      | Git / GitHub  |

---

# 📚 Arduino Libraries

The firmware uses the following Arduino libraries/interfaces:

```text
Wire
SPI
SD
OneWire
SoftwareSerial
Math
```

Sensor-specific libraries required by the implementation should be installed through the Arduino IDE Library Manager where applicable.

---

# 🚀 Getting Started

## 1. Clone the Repository

```bash
git clone https://github.com/Tanish-str/HAB-High-Altitude-Balloon.git
cd HAB-High-Altitude-Balloon
```

---

## 2. Open the Firmware

Open the flight-computer sketch using Arduino IDE.

```text
HAB_Flight_Computer/
└── HAB_Flight_Computer.ino
```

---

## 3. Select the Board

In Arduino IDE:

```text
Board:
Arduino Nano

Processor:
ATmega328P
```

Select the COM port corresponding to the connected Arduino Nano.

---

## 4. Verify Hardware

Before uploading the flight firmware, verify:

* Sensor wiring
* I²C connections
* I²C addresses
* SD card connections
* GPS connections
* LoRa connections
* E32 AUX connection
* Battery voltage-divider values
* LoRa antenna connection
* Correct Nano processor selection

---

## 5. Upload Firmware

Connect the Arduino Nano using USB and upload the firmware.

After uploading, verify the initialization messages through the Serial Monitor.

---

# 🔬 Hardware Bring-Up Sequence

For reliable testing, hardware can be brought up incrementally:

```text
Arduino Nano
     ↓
I²C Bus
     ↓
BMP180 / AHT10 / MPU6050
     ↓
DS18B20
     ↓
GPS
     ↓
SD Card
     ↓
LoRa
     ↓
Complete Telemetry System
```

Testing subsystems individually makes it easier to identify wiring, communication, or sensor initialization problems before integrating the complete flight computer.

---

# 🧪 Ground Testing

Before flight, verify the following:

### Sensor Test

* BMP180 returns valid pressure and temperature
* AHT10 returns valid temperature and humidity
* MPU6050 returns acceleration and gyroscope values
* Both DS18B20 sensors return temperature
* UV sensor produces an analog reading
* Battery voltage measurement is within expected range

### GPS Test

* GPS receives a valid fix
* Latitude and longitude are reasonable
* Satellite count is updating
* GPS freshness logic behaves correctly

### SD Test

* SD card initializes
* `HABLOG.CSV` is created
* Telemetry rows are written successfully
* Data remains readable after power cycling

### LoRa Test

* E32 initializes
* AUX status behaves correctly
* Packets are transmitted
* Ground station receives packets
* Checksum validation succeeds
* Invalid packets are rejected

---

# 🔗 Ground Station Integration

The onboard flight computer connects to the ground system through the LoRa telemetry link.

```text
                 ONBOARD
┌───────────────────────────────┐
│ Arduino Nano                  │
│                               │
│ Sensors → Processing → LoRa   │
└────────────────┬──────────────┘
                 │
                 │ 433 MHz
                 ▼
┌───────────────────────────────┐
│ Ground Station Arduino Uno    │
│                               │
│ LoRa → Packet Validation      │
│       → CSV Serial Output     │
└────────────────┬──────────────┘
                 │
                 │ USB
                 ▼
┌───────────────────────────────┐
│ Serial Bridge                 │
│                               │
│ CSV → Structured Telemetry    │
└────────────────┬──────────────┘
                 │
                 ▼
┌───────────────────────────────┐
│ Backend + SQLite              │
└────────────────┬──────────────┘
                 │
                 ▼
┌───────────────────────────────┐
│ HAB01 Mission Dashboard       │
└───────────────────────────────┘
```

The dashboard is therefore an extension of the hardware telemetry system rather than a standalone visualization project.

---

# 📁 Recommended Repository Structure

The overall HAB repository can be organized as:

```text
HAB-High-Altitude-Balloon/
│
├── firmware/
│   │
│   ├── flight-computer/
│   │   └── HAB_Flight_Computer.ino
│   │
│   └── ground-station/
│       └── HAB01_ground_station.ino
│
├── dashboard/
│   ├── backend/
│   ├── bridge/
│   ├── frontend/
│   ├── data/
│   └── README.md
│
├── hardware/
│   ├── wiring/
│   ├── schematics/
│   ├── pinouts/
│   └── images/
│
├── documentation/
│   ├── mission/
│   ├── telemetry/
│   └── testing/
│
├── LICENSE
└── README.md
```

---

# 🛣️ Hardware & Firmware Roadmap

Future improvements include:

* [ ] Watchdog-based recovery
* [ ] Improved flight-event detection
* [ ] Advanced battery health monitoring
* [ ] Telemetry compression
* [ ] Stronger CRC packet validation
* [ ] Power optimization
* [ ] Fault-tolerant sensor recovery
* [ ] Additional environmental sensors
* [ ] Improved thermal monitoring
* [ ] Automated post-flight data analysis
* [ ] Additional mission telemetry parameters

---

# 📊 Complete HAB01 System

The project combines embedded hardware, wireless communication, data processing, and software visualization into a single telemetry pipeline:

```text
┌─────────────────────────────────────────────────────────────┐
│                     HAB01 MISSION                           │
└─────────────────────────────────────────────────────────────┘

        PAYLOAD / FLIGHT COMPUTER
                   │
       ┌───────────┴───────────┐
       │                       │
    Sensors                 GPS
       │                       │
       └───────────┬───────────┘
                   ▼
             Arduino Nano
                   │
          ┌────────┴────────┐
          │                 │
       SD Card             LoRa
          │                 │
          ▼                 ▼
   HABLOG.CSV        Ground Station
                            │
                            ▼
                      Serial Bridge
                            │
                            ▼
                    Backend + SQLite
                            │
                            ▼
                   Live HAB Dashboard
```

This creates an end-to-end system for:

**Sensing → Processing → Logging → Transmission → Reception → Storage → Visualization**

---

# 📄 License

This project is licensed under the **MIT License**.

---

# ⭐ Acknowledgements

The HAB01 Flight Computer was developed as part of a High Altitude Balloon mission focused on building an integrated embedded telemetry and data-logging platform.

The system combines an **Arduino Nano**, environmental sensors, GPS, motion sensing, microSD storage, and an **EBYTE E32-433T30D LoRa communication system** to provide onboard telemetry throughout the mission.
