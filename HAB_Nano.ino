/*
  ===========================================================================
   HAB01 — Arduino Nano Standalone Flight Computer / Sensor Payload
  ===========================================================================
  Sensors:
    - BMP180    (I2C)          temperature + pressure + derived altitude
    - AHT10     (I2C)          temperature + humidity
    - MPU6050   (I2C)          accelerometer + gyroscope (no on-chip temp used)
    - DS18B20 x2 (OneWire)     one INSIDE the payload box, one OUTSIDE
    - GUVA-S12SD (analog)      UV index
    - NEO-8M / NEO-M8L GPS     (SoftwareSerial, NMEA)
    - SD card module (SPI)     CSV logging

  Derived / computed values:
    - Barometric altitude (AGL, referenced to ground pressure at boot)
    - Acceleration magnitude (vector sum of MPU6050 accel axes)
    - Temperature gradient (rate of change of outside temp, C/s)
    - Inside/outside temperature difference
    - Flight phase state machine (PRELAUNCH/ASCENT/NEAR_APOGEE/DESCENT/LANDED)
    - Maximum altitude reached
    - GPS altitude vs barometric altitude difference
    - Sensor health bitmask

  Libraries required (Library Manager):
    - OneWire        (Paul Stoffregen)
    - TinyGPSPlus     (Mikal Hart)
    - SD, SPI, Wire, SoftwareSerial (bundled with Arduino IDE)

  ===========================================================================
   WIRING
  ===========================================================================
  I2C bus (shared by BMP180, AHT10, MPU6050):
    SDA  -> Nano A4
    SCL  -> Nano A5
    VCC  -> 5V   (breakout boards handle regulation/level-shifting)
    GND  -> GND

  DS18B20 #1 — INSIDE the payload box:
    DATA -> Nano D4  (+ 4.7k pull-up resistor from D4 to 5V)
    VCC  -> 5V, GND -> GND

  DS18B20 #2 — OUTSIDE, exposed to ambient air:
    DATA -> Nano D5  (+ 4.7k pull-up resistor from D5 to 5V)
    VCC  -> 5V, GND -> GND
    (Two separate OneWire buses/pins are used instead of ROM addressing on a
     shared bus — simpler to wire correctly and to reason about in code.)

  GUVA-S12SD (UV sensor, analog):
    OUT  -> Nano A0
    VCC  -> 5V, GND -> GND

  NEO-8M / NEO-M8L GPS (SoftwareSerial @ 9600):
    GPS TX -> Nano D2   (SoftwareSerial RX)
    GPS RX -> Nano D3   (SoftwareSerial TX) -- add a voltage divider
              (e.g. 10k + 20k) if your GPS module's RX pin is rated
              strictly 3.3V.
    VCC    -> 5V, GND -> GND

  SD card module (hardware SPI):
    CS   -> Nano D10
    MOSI -> Nano D11
    MISO -> Nano D12
    SCK  -> Nano D13
    VCC  -> 5V, GND -> GND

  Common GND across every module is mandatory.

  IMPORTANT CAVEAT FOR HIGH-ALTITUDE BALLOON USE:
    The BMP180 is only rated/accurate roughly 300-1100 hPa (~0-9000 m).
    Above that its readings become unreliable. If this payload is going
    above ~9-10 km, treat barometric altitude as informational only near
    apogee and trust GPS altitude instead. Consider a BMP280/BME280 or
    MS5611 for a real high-altitude flight (see suggestions at the bottom
    of the assistant's reply).
  ===========================================================================
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <TinyGPSPlus.h>
#include <math.h>

/* ---------------------------- Pin definitions ---------------------------- */
#define ONEWIRE_IN_PIN   4   /* DS18B20 inside the box  */
#define ONEWIRE_OUT_PIN  5   /* DS18B20 outside the box */
#define UV_PIN           A0
#define SD_CS_PIN        10
#define GPS_RX_PIN       2   /* Nano RX  <- GPS TX */
#define GPS_TX_PIN       3   /* Nano TX  -> GPS RX */

/* ---------------------------- I2C addresses ------------------------------ */
#define BMP180_ADDR   0x77
#define AHT10_ADDR    0x38
#define MPU6050_ADDR  0x68

/* BMP180 registers */
#define BMP180_REG_CALIB_START  0xAA
#define BMP180_REG_CONTROL      0xF4
#define BMP180_REG_RESULT       0xF6
#define BMP180_CMD_TEMP         0x2E
#define BMP180_CMD_PRESS_OSS0   0x34

/* AHT10 commands */
#define AHT10_CMD_INIT       0xE1
#define AHT10_CMD_TRIGGER    0xAC

/* MPU6050 registers */
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_ACCEL_XOUT  0x3B
#define MPU6050_REG_GYRO_XOUT   0x43

/* DS18B20 commands */
#define DS18B20_CMD_SKIP_ROM     0xCC
#define DS18B20_CMD_CONVERT_T    0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE

/* UV sensor */
#define UV_ADC_VREF      5.0f
#define UV_ADC_MAXCOUNT  1023.0f

/* Flight phase thresholds — TUNE THESE to your expected flight profile */
#define LAUNCH_CLIMB_THRESHOLD   1.0f    /* m/s climb rate to leave PRELAUNCH  */
#define APOGEE_CLIMB_THRESHOLD   0.5f    /* m/s |climb rate| considered "flat" */
#define APOGEE_ALT_WINDOW        50.0f   /* m below max-alt to still call it near-apogee */
#define DESCENT_THRESHOLD       -1.0f    /* m/s climb rate to declare DESCENT  */
#define LANDED_ALT_CHANGE        1.0f    /* m altitude change considered "still" */
#define LANDED_CONFIRM_CYCLES    5        /* consecutive still cycles -> LANDED */

/* Flight phase enum */
enum FlightPhase { PRELAUNCH = 0, ASCENT = 1, NEAR_APOGEE = 2, DESCENT = 3, LANDED = 4 };
const char *PHASE_NAMES[5] = { "PRELAUNCH", "ASCENT", "NEAR_APOGEE", "DESCENT", "LANDED" };

/* ---------------------------- Globals ------------------------------------ */
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
TinyGPSPlus gps;
OneWire dsIn(ONEWIRE_IN_PIN);
OneWire dsOut(ONEWIRE_OUT_PIN);
File logFile;

/* BMP180 calibration coefficients */
int16_t  AC1, AC2, AC3, VB1, VB2, MB, MC, MD;
uint16_t AC4, AC5, AC6;
float groundPressure_Pa = 101325.0f; /* captured at boot, used as AGL reference */

/* Sensor data */
struct { float tempC; long pressPa; float altitude_m; bool ok; } bmp;
struct { float ax_g, ay_g, az_g, gx_dps, gy_dps, gz_dps, accelMag_g; bool ok; } mpu;
struct { float tempC, humPct; bool ok; } aht;
struct { float tempC; bool ok; } dsIn_data;
struct { float tempC; bool ok; } dsOut_data;
float uv_index = 0.0f;

/* Derived values */
float tempDiff_C        = 0.0f;   /* inside - outside */
float tempGradient_Cps  = 0.0f;   /* outside temp rate of change, C/s */
float maxAltitude_m     = -100000.0f;
float gpsBaroAltDiff_m  = 0.0f;
uint8_t flightPhase     = PRELAUNCH;
uint8_t sensorHealthMask = 0;

/* Health bitmask positions */
#define HEALTH_BMP180   0
#define HEALTH_MPU6050  1
#define HEALTH_AHT10    2
#define HEALTH_DS_IN    3
#define HEALTH_DS_OUT   4
#define HEALTH_GPS_FIX  5
#define HEALTH_SD_WRITE 6

/* State tracking for derived calculations */
float   prevAltitude_m   = 0.0f;
float   prevOutsideTemp  = 0.0f;
unsigned long prevTime_ms = 0;
float   climbRate_mps    = 0.0f;
uint8_t landedConfirmCount = 0;
bool    firstCycle        = true;

unsigned long lastCycle = 0;
const unsigned long CYCLE_MS = 2000;

/* =========================================================================
   BMP180
   ========================================================================= */
bool BMP180_ReadCalibration()
{
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_REG_CALIB_START);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(BMP180_ADDR, 22);
  if (Wire.available() < 22) return false;

  uint8_t b[22];
  for (int i = 0; i < 22; i++) b[i] = Wire.read();

  AC1 = (int16_t)((b[0] << 8) | b[1]);
  AC2 = (int16_t)((b[2] << 8) | b[3]);
  AC3 = (int16_t)((b[4] << 8) | b[5]);
  AC4 = (uint16_t)((b[6] << 8) | b[7]);
  AC5 = (uint16_t)((b[8] << 8) | b[9]);
  AC6 = (uint16_t)((b[10] << 8) | b[11]);
  VB1 = (int16_t)((b[12] << 8) | b[13]);
  VB2 = (int16_t)((b[14] << 8) | b[15]);
  MB  = (int16_t)((b[16] << 8) | b[17]);
  MC  = (int16_t)((b[18] << 8) | b[19]);
  MD  = (int16_t)((b[20] << 8) | b[21]);
  return true;
}

bool BMP180_ReadRaw(long *rawTemp, long *rawPress)
{
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_REG_CONTROL);
  Wire.write(BMP180_CMD_TEMP);
  if (Wire.endTransmission() != 0) return false;
  delay(5);

  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_REG_RESULT);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(BMP180_ADDR, 2);
  if (Wire.available() < 2) return false;
  *rawTemp = ((long)Wire.read() << 8) | Wire.read();

  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_REG_CONTROL);
  Wire.write(BMP180_CMD_PRESS_OSS0);
  if (Wire.endTransmission() != 0) return false;
  delay(5);

  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_REG_RESULT);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(BMP180_ADDR, 3);
  if (Wire.available() < 3) return false;
  uint8_t p0 = Wire.read(), p1 = Wire.read(), p2 = Wire.read();
  *rawPress = (((long)p0 << 16) | ((long)p1 << 8) | p2) >> 8;
  return true;
}

bool BMP180_ReadData()
{
  long UT, UP, X1, X2, X3, B3, B5, B6, p;
  unsigned long B4, B7;

  if (!BMP180_ReadRaw(&UT, &UP)) { bmp.ok = false; return false; }

  X1 = ((UT - (long)AC6) * (long)AC5) >> 15;
  X2 = ((long)MC << 11) / (X1 + MD);
  B5 = X1 + X2;
  bmp.tempC = ((B5 + 8) >> 4) / 10.0f;

  B6 = B5 - 4000;
  X1 = (VB2 * (B6 * B6 >> 12)) >> 11;
  X2 = (AC2 * B6) >> 11;
  X3 = X1 + X2;
  B3 = ((((long)AC1 * 4 + X3)) + 2) >> 2;
  X1 = (AC3 * B6) >> 13;
  X2 = (VB1 * (B6 * B6 >> 12)) >> 16;
  X3 = ((X1 + X2) + 2) >> 2;
  B4 = (AC4 * (unsigned long)(X3 + 32768)) >> 15;
  B7 = ((unsigned long)UP - B3) * 50000;

  if (B7 < 0x80000000UL) p = (B7 * 2) / B4;
  else p = (B7 / B4) * 2;

  X1 = (p >> 8) * (p >> 8);
  X1 = (X1 * 3038) >> 16;
  X2 = (-7357 * p) >> 16;
  p = p + ((X1 + X2 + 3791) >> 4);

  bmp.pressPa = p;

  /* Altitude above ground level, referenced to groundPressure_Pa captured at boot */
  bmp.altitude_m = 44330.0f * (1.0f - pow((float)p / groundPressure_Pa, 0.1903f));

  bmp.ok = true;
  return true;
}

/* =========================================================================
   AHT10
   ========================================================================= */
bool AHT10_Init()
{
  Wire.beginTransmission(AHT10_ADDR);
  Wire.write(AHT10_CMD_INIT);
  Wire.write((uint8_t)0x08);
  Wire.write((uint8_t)0x00);
  bool ok = (Wire.endTransmission() == 0);
  delay(20);
  return ok;
}

bool AHT10_ReadData()
{
  Wire.beginTransmission(AHT10_ADDR);
  Wire.write(AHT10_CMD_TRIGGER);
  Wire.write((uint8_t)0x33);
  Wire.write((uint8_t)0x00);
  if (Wire.endTransmission() != 0) { aht.ok = false; return false; }

  delay(80);
  Wire.requestFrom(AHT10_ADDR, 6);
  if (Wire.available() < 6) { aht.ok = false; return false; }

  uint8_t d[6];
  for (int i = 0; i < 6; i++) d[i] = Wire.read();

  uint32_t rawHum  = ((uint32_t)d[1] << 12) | ((uint32_t)d[2] << 4) | (d[3] >> 4);
  uint32_t rawTemp = ((uint32_t)(d[3] & 0x0F) << 16) | ((uint32_t)d[4] << 8) | d[5];

  aht.humPct = ((float)rawHum / 1048576.0f) * 100.0f;
  aht.tempC  = ((float)rawTemp / 1048576.0f) * 200.0f - 50.0f;
  aht.ok = true;
  return true;
}

/* =========================================================================
   MPU6050  (accel + gyro only — temperature intentionally not read/used)
   ========================================================================= */
bool MPU6050_Init()
{
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_REG_PWR_MGMT_1);
  Wire.write((uint8_t)0x00); /* wake up from sleep */
  return (Wire.endTransmission() == 0);
}

bool MPU6050_ReadData()
{
  /* Accel: registers 0x3B-0x40 */
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_REG_ACCEL_XOUT);
  if (Wire.endTransmission(false) != 0) { mpu.ok = false; return false; }
  Wire.requestFrom(MPU6050_ADDR, 6);
  if (Wire.available() < 6) { mpu.ok = false; return false; }
  int16_t rawAX = (Wire.read() << 8) | Wire.read();
  int16_t rawAY = (Wire.read() << 8) | Wire.read();
  int16_t rawAZ = (Wire.read() << 8) | Wire.read();

  /* Gyro: registers 0x43-0x48 (skips the temp registers 0x41-0x42 in between) */
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_REG_GYRO_XOUT);
  if (Wire.endTransmission(false) != 0) { mpu.ok = false; return false; }
  Wire.requestFrom(MPU6050_ADDR, 6);
  if (Wire.available() < 6) { mpu.ok = false; return false; }
  int16_t rawGX = (Wire.read() << 8) | Wire.read();
  int16_t rawGY = (Wire.read() << 8) | Wire.read();
  int16_t rawGZ = (Wire.read() << 8) | Wire.read();

  /* Default full-scale ranges: accel +/-2g (16384 LSB/g), gyro +/-250 dps (131 LSB/dps) */
  mpu.ax_g   = rawAX / 16384.0f;
  mpu.ay_g   = rawAY / 16384.0f;
  mpu.az_g   = rawAZ / 16384.0f;
  mpu.gx_dps = rawGX / 131.0f;
  mpu.gy_dps = rawGY / 131.0f;
  mpu.gz_dps = rawGZ / 131.0f;

  mpu.accelMag_g = sqrt(mpu.ax_g * mpu.ax_g + mpu.ay_g * mpu.ay_g + mpu.az_g * mpu.az_g);

  mpu.ok = true;
  return true;
}

/* =========================================================================
   DS18B20 (via OneWire library) — generic reader, takes which bus to use
   ========================================================================= */
bool DS18B20_ReadTemperature(OneWire &bus, float *tempC)
{
  if (!bus.reset()) return false;
  bus.skip();
  bus.write(DS18B20_CMD_CONVERT_T);

  delay(750); /* max conversion time at 12-bit resolution */

  if (!bus.reset()) return false;
  bus.skip();
  bus.write(DS18B20_CMD_READ_SCRATCH);

  uint8_t lsb = bus.read();
  uint8_t msb = bus.read();

  int16_t raw = (int16_t)((msb << 8) | lsb);
  *tempC = (float)raw / 16.0f;
  return true;
}

/* =========================================================================
   UV sensor (GUVA-S12SD, A0)
   ========================================================================= */
float ADC_ReadUVIndex()
{
  int adcVal = analogRead(UV_PIN);
  float voltage = (adcVal / UV_ADC_MAXCOUNT) * UV_ADC_VREF;
  /* GUVA-S12SD: roughly 0.1 V per UV-index unit */
  return voltage / 0.1f;
}

/* =========================================================================
   GPS feeding (call often from loop to keep TinyGPS++ fed)
   ========================================================================= */
void GPS_Feed()
{
  while (gpsSerial.available() > 0)
  {
    gps.encode(gpsSerial.read());
  }
}

/* =========================================================================
   Derived calculations: climb rate, gradient, diffs, max altitude, phase
   ========================================================================= */
void UpdateDerivedValues()
{
  unsigned long now = millis();
  float dt_s = firstCycle ? (CYCLE_MS / 1000.0f) : (now - prevTime_ms) / 1000.0f;
  if (dt_s <= 0.0f) dt_s = CYCLE_MS / 1000.0f;

  /* Climb rate from barometric altitude */
  if (bmp.ok)
  {
    climbRate_mps = firstCycle ? 0.0f : (bmp.altitude_m - prevAltitude_m) / dt_s;
    if (bmp.altitude_m > maxAltitude_m) maxAltitude_m = bmp.altitude_m;
  }

  /* Temperature gradient — rate of change of OUTSIDE temperature */
  if (dsOut_data.ok)
  {
    tempGradient_Cps = firstCycle ? 0.0f : (dsOut_data.tempC - prevOutsideTemp) / dt_s;
  }

  /* Inside vs outside temperature difference */
  if (dsIn_data.ok && dsOut_data.ok)
  {
    tempDiff_C = dsIn_data.tempC - dsOut_data.tempC;
  }

  /* GPS altitude vs barometric altitude (both must be valid) */
  if (bmp.ok && gps.altitude.isValid())
  {
    gpsBaroAltDiff_m = gps.altitude.meters() - bmp.altitude_m;
  }

  /* --- Flight phase state machine --- */
  if (bmp.ok)
  {
    switch (flightPhase)
    {
      case PRELAUNCH:
        if (climbRate_mps > LAUNCH_CLIMB_THRESHOLD) flightPhase = ASCENT;
        break;

      case ASCENT:
        if (climbRate_mps < DESCENT_THRESHOLD)
        {
          flightPhase = DESCENT; /* handles a fast balloon burst */
        }
        else if (fabs(climbRate_mps) < APOGEE_CLIMB_THRESHOLD &&
                 bmp.altitude_m > (maxAltitude_m - APOGEE_ALT_WINDOW))
        {
          flightPhase = NEAR_APOGEE;
        }
        break;

      case NEAR_APOGEE:
        if (climbRate_mps < DESCENT_THRESHOLD) flightPhase = DESCENT;
        else if (climbRate_mps > LAUNCH_CLIMB_THRESHOLD) flightPhase = ASCENT;
        break;

      case DESCENT:
        if (fabs(bmp.altitude_m - prevAltitude_m) < LANDED_ALT_CHANGE)
        {
          landedConfirmCount++;
          if (landedConfirmCount >= LANDED_CONFIRM_CYCLES) flightPhase = LANDED;
        }
        else
        {
          landedConfirmCount = 0;
        }
        break;

      case LANDED:
        /* stays LANDED; add movement-detection here if you need re-arming */
        break;
    }
  }

  /* Roll state forward */
  if (bmp.ok)     prevAltitude_m  = bmp.altitude_m;
  if (dsOut_data.ok) prevOutsideTemp = dsOut_data.tempC;
  prevTime_ms = now;
  firstCycle = false;
}

/* =========================================================================
   Sensor health bitmask
   ========================================================================= */
void UpdateSensorHealth()
{
  sensorHealthMask = 0;
  if (bmp.ok)               sensorHealthMask |= (1 << HEALTH_BMP180);
  if (mpu.ok)                sensorHealthMask |= (1 << HEALTH_MPU6050);
  if (aht.ok)                sensorHealthMask |= (1 << HEALTH_AHT10);
  if (dsIn_data.ok)          sensorHealthMask |= (1 << HEALTH_DS_IN);
  if (dsOut_data.ok)         sensorHealthMask |= (1 << HEALTH_DS_OUT);
  if (gps.location.isValid()) sensorHealthMask |= (1 << HEALTH_GPS_FIX);
  /* HEALTH_SD_WRITE bit is set by SD_LogRow() itself, after a successful write */
}

/* =========================================================================
   SD logging
   ========================================================================= */
bool SD_WriteHeaderIfNew()
{
  if (!SD.exists("HABLOG.CSV"))
  {
    File f = SD.open("HABLOG.CSV", FILE_WRITE);
    if (!f) return false;
    f.println(F("millis,bmp_tempC,bmp_pressPa,bmp_altitude_m,"
                 "mpu_ax,mpu_ay,mpu_az,mpu_gx,mpu_gy,mpu_gz,accel_mag_g,"
                 "aht_tempC,aht_humPct,ds_in_tempC,ds_out_tempC,temp_diff_C,temp_gradient_Cps,"
                 "uv_index,flight_phase,max_altitude_m,"
                 "gps_valid,gps_lat,gps_lon,gps_alt_m,gps_baro_alt_diff_m,gps_speed_kmh,gps_date,gps_time,"
                 "sensor_health_hex"));
    f.close();
  }
  return true;
}

void SD_LogRow()
{
  logFile = SD.open("HABLOG.CSV", FILE_WRITE);
  if (!logFile) return;

  logFile.print(millis());          logFile.print(',');

  if (bmp.ok) { logFile.print(bmp.tempC, 2); logFile.print(','); logFile.print(bmp.pressPa); logFile.print(','); logFile.print(bmp.altitude_m, 1); }
  else        { logFile.print(F("NA,NA,NA")); }
  logFile.print(',');

  if (mpu.ok)
  {
    logFile.print(mpu.ax_g, 3); logFile.print(',');
    logFile.print(mpu.ay_g, 3); logFile.print(',');
    logFile.print(mpu.az_g, 3); logFile.print(',');
    logFile.print(mpu.gx_dps, 2); logFile.print(',');
    logFile.print(mpu.gy_dps, 2); logFile.print(',');
    logFile.print(mpu.gz_dps, 2); logFile.print(',');
    logFile.print(mpu.accelMag_g, 3);
  }
  else { logFile.print(F("NA,NA,NA,NA,NA,NA,NA")); }
  logFile.print(',');

  if (aht.ok) { logFile.print(aht.tempC, 2); logFile.print(','); logFile.print(aht.humPct, 2); }
  else        { logFile.print(F("NA,NA")); }
  logFile.print(',');

  if (dsIn_data.ok)  logFile.print(dsIn_data.tempC, 2);  else logFile.print(F("NA"));
  logFile.print(',');
  if (dsOut_data.ok) logFile.print(dsOut_data.tempC, 2); else logFile.print(F("NA"));
  logFile.print(',');

  if (dsIn_data.ok && dsOut_data.ok) logFile.print(tempDiff_C, 2); else logFile.print(F("NA"));
  logFile.print(',');
  if (dsOut_data.ok) logFile.print(tempGradient_Cps, 4); else logFile.print(F("NA"));
  logFile.print(',');

  logFile.print(uv_index, 2);
  logFile.print(',');

  logFile.print(flightPhase);
  logFile.print(',');
  logFile.print(maxAltitude_m, 1);
  logFile.print(',');

  if (gps.location.isValid())
  {
    logFile.print(F("1,"));
    logFile.print(gps.location.lat(), 6); logFile.print(',');
    logFile.print(gps.location.lng(), 6); logFile.print(',');
    logFile.print(gps.altitude.meters(), 1); logFile.print(',');
    if (bmp.ok) logFile.print(gpsBaroAltDiff_m, 1); else logFile.print(F("NA"));
    logFile.print(',');
    logFile.print(gps.speed.kmph(), 2); logFile.print(',');
    if (gps.date.isValid()) { logFile.print(gps.date.month()); logFile.print('/'); logFile.print(gps.date.day()); logFile.print('/'); logFile.print(gps.date.year()); }
    else logFile.print(F("NA"));
    logFile.print(',');
    if (gps.time.isValid()) { logFile.print(gps.time.hour()); logFile.print(':'); logFile.print(gps.time.minute()); logFile.print(':'); logFile.print(gps.time.second()); }
    else logFile.print(F("NA"));
  }
  else
  {
    logFile.print(F("0,NA,NA,NA,NA,NA,NA,NA"));
  }
  logFile.print(',');

  logFile.print(F("0x"));
  logFile.println(sensorHealthMask, HEX);

  logFile.close();
  sensorHealthMask |= (1 << HEALTH_SD_WRITE); /* mark this cycle's write as successful */
}

/* =========================================================================
   Serial report (Tera Term)
   ========================================================================= */
void PrintReport()
{
  Serial.println(F("\r\n--- HAB01 Sensor Report ---"));

  if (bmp.ok) {
    Serial.print(F("BMP180   : Temp = ")); Serial.print(bmp.tempC, 2);
    Serial.print(F(" C, Pressure = ")); Serial.print(bmp.pressPa);
    Serial.print(F(" Pa, Altitude = ")); Serial.print(bmp.altitude_m, 1); Serial.println(F(" m AGL"));
  } else Serial.println(F("BMP180   : read error"));

  if (mpu.ok) {
    Serial.print(F("MPU6050  : Acc[g] X=")); Serial.print(mpu.ax_g, 3);
    Serial.print(F(" Y=")); Serial.print(mpu.ay_g, 3);
    Serial.print(F(" Z=")); Serial.print(mpu.az_g, 3);
    Serial.print(F(" |Mag|=")); Serial.print(mpu.accelMag_g, 3);
    Serial.print(F(" g | Gyro[dps] X=")); Serial.print(mpu.gx_dps, 2);
    Serial.print(F(" Y=")); Serial.print(mpu.gy_dps, 2);
    Serial.print(F(" Z=")); Serial.println(mpu.gz_dps, 2);
  } else Serial.println(F("MPU6050  : read error"));

  if (aht.ok) { Serial.print(F("AHT10    : Temp = ")); Serial.print(aht.tempC, 2);
                Serial.print(F(" C, Humidity = ")); Serial.print(aht.humPct, 2); Serial.println(F(" %")); }
  else          Serial.println(F("AHT10    : read error"));

  if (dsIn_data.ok)  { Serial.print(F("DS18B20 IN : Temp = ")); Serial.print(dsIn_data.tempC, 2); Serial.println(F(" C")); }
  else                 Serial.println(F("DS18B20 IN : read error"));
  if (dsOut_data.ok) { Serial.print(F("DS18B20 OUT: Temp = ")); Serial.print(dsOut_data.tempC, 2); Serial.println(F(" C")); }
  else                 Serial.println(F("DS18B20 OUT: read error"));

  if (dsIn_data.ok && dsOut_data.ok) {
    Serial.print(F("Temp diff (in-out): ")); Serial.print(tempDiff_C, 2); Serial.println(F(" C"));
  }
  if (dsOut_data.ok) {
    Serial.print(F("Temp gradient (outside): ")); Serial.print(tempGradient_Cps, 4); Serial.println(F(" C/s"));
  }

  Serial.print(F("UV Sensor: Index = ")); Serial.println(uv_index, 2);

  Serial.print(F("Flight phase: ")); Serial.print(flightPhase);
  Serial.print(F(" (")); Serial.print(PHASE_NAMES[flightPhase]); Serial.println(F(")"));
  Serial.print(F("Climb rate: ")); Serial.print(climbRate_mps, 2); Serial.println(F(" m/s"));
  Serial.print(F("Max altitude so far: ")); Serial.print(maxAltitude_m, 1); Serial.println(F(" m"));

  if (gps.location.isValid())
  {
    Serial.print(F("GPS      : Fix OK | "));
    if (gps.time.isValid()) { Serial.print(gps.time.hour()); Serial.print(':'); Serial.print(gps.time.minute()); Serial.print(':'); Serial.print(gps.time.second()); }
    Serial.print(F(" UTC | Lat = ")); Serial.print(gps.location.lat(), 6);
    Serial.print(F("  Lon = ")); Serial.print(gps.location.lng(), 6);
    Serial.print(F(" | GPS Alt = ")); Serial.print(gps.altitude.meters(), 1);
    Serial.print(F(" m | Baro-GPS diff = ")); Serial.print(gpsBaroAltDiff_m, 1);
    Serial.print(F(" m | Speed = ")); Serial.print(gps.speed.kmph(), 2);
    Serial.println(F(" km/h"));
  }
  else
  {
    Serial.print(F("GPS      : no valid fix yet (satellites seen: "));
    Serial.print(gps.satellites.value());
    Serial.println(F(")"));
  }

  Serial.print(F("Sensor health: 0x")); Serial.print(sensorHealthMask, HEX);
  Serial.print(F("  [BMP180=")); Serial.print(bmp.ok ? F("OK") : F("FAIL"));
  Serial.print(F(" MPU6050=")); Serial.print(mpu.ok ? F("OK") : F("FAIL"));
  Serial.print(F(" AHT10=")); Serial.print(aht.ok ? F("OK") : F("FAIL"));
  Serial.print(F(" DS_IN=")); Serial.print(dsIn_data.ok ? F("OK") : F("FAIL"));
  Serial.print(F(" DS_OUT=")); Serial.print(dsOut_data.ok ? F("OK") : F("FAIL"));
  Serial.print(F(" GPS=")); Serial.print(gps.location.isValid() ? F("OK") : F("NO FIX"));
  Serial.println(F("]"));
}

/* =========================================================================
   Setup / Loop
   ========================================================================= */
void setup()
{
  Serial.begin(115200);
  gpsSerial.begin(9600);
  Wire.begin();

  Serial.println(F("\r\n===== HAB01 Nano Payload Boot ====="));

  bool bmpCalOk = BMP180_ReadCalibration();
  if (bmpCalOk) Serial.println(F("BMP180 calibration OK"));
  else            Serial.println(F("BMP180 calibration FAILED"));

  if (MPU6050_Init()) Serial.println(F("MPU6050 init OK"));
  else                  Serial.println(F("MPU6050 init FAILED"));

  if (AHT10_Init()) Serial.println(F("AHT10 init OK"));
  else                Serial.println(F("AHT10 init FAILED"));

  if (SD.begin(SD_CS_PIN))
  {
    Serial.println(F("SD card init OK"));
    SD_WriteHeaderIfNew();
  }
  else
  {
    Serial.println(F("SD card init FAILED — logging disabled"));
  }

  /* Capture ground-level pressure as the AGL reference (average a few reads) */
  if (bmpCalOk)
  {
    long sumPa = 0;
    uint8_t n = 0;
    for (uint8_t i = 0; i < 5; i++)
    {
      if (BMP180_ReadData()) { sumPa += bmp.pressPa; n++; }
      delay(100);
    }
    if (n > 0)
    {
      groundPressure_Pa = (float)sumPa / n;
      Serial.print(F("Ground pressure reference: "));
      Serial.print(groundPressure_Pa, 1);
      Serial.println(F(" Pa"));
    }
  }

  lastCycle = millis();
  prevTime_ms = millis();
}

void loop()
{
  /* Keep feeding the GPS parser continuously, not just once per cycle */
  GPS_Feed();

  if (millis() - lastCycle >= CYCLE_MS)
  {
    lastCycle = millis();

    BMP180_ReadData();
    MPU6050_ReadData();
    AHT10_ReadData();
    dsIn_data.ok  = DS18B20_ReadTemperature(dsIn, &dsIn_data.tempC);
    dsOut_data.ok = DS18B20_ReadTemperature(dsOut, &dsOut_data.tempC);
    uv_index = ADC_ReadUVIndex();

    UpdateDerivedValues();
    UpdateSensorHealth();

    PrintReport();
    SD_LogRow();
  }
}
