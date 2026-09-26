/*
  ===========================================================================
   HAB01 — Ground Station Receiver (Arduino Nano + E32-433T30D)
  ===========================================================================
   Receives the 59-byte binary TelemetryFrame sent by the flight computer,
   validates it (sync bytes + XOR checksum), decodes every field back to
   engineering units, and prints one CSV row per received frame over USB
   to your PC (Serial Monitor, or redirect into a logging/plotting tool).

   IMPORTANT DESIGN CHOICE — opposite of the flight computer:
     Here the E32 is on SoftwareSerial (D2/D3), NOT the hardware UART.
     That leaves D0/D1 (the USB-Serial bridge) completely free for talking
     to your PC, so you get live CSV output over Serial Monitor with zero
     conflict and no need to disconnect anything to reprogram this board.
     (On the flight computer this trade-off went the other way, because
     GPS needed a software port there instead.)

   The struct below MUST stay byte-for-byte identical to the transmitter's
   TelemetryFrame — same field order, same types, same #pragma pack(1).
  ===========================================================================
   WIRING
  ===========================================================================
   E32-433T30D:
     E32 TX  -> Nano D2   (SoftwareSerial RX)
     E32 RX  -> Nano D3   (SoftwareSerial TX) -- add a voltage divider
                (e.g. 10k+20k) if your module's RX pin is 3.3V-only.
     M0      -> GND   (tied low, together with M1, selects Mode 0 —
     M1      -> GND    Normal / transparent transmission)
     AUX     -> Nano D6  (not required for receiving, but wired for
                          future use / status monitoring; unused by
                          this sketch's logic)
     VCC     -> 5V, with a 100–470uF capacitor right at VCC/GND
                (same current-spike reasoning as the transmitter side)
     GND     -> common GND

   USB (D0/D1) -> PC, for CSV output. Free at all times on this board —
   nothing else uses it, so no disconnect-before-upload dance here.
  ===========================================================================
*/

#include <SoftwareSerial.h>

#define PROJECT_BAUD   9600   /* USB link to PC */
#define LORA_UART_BAUD 9600   /* must match the E32 module's configured UART baud, both ends */

#define LORA_RX_PIN  2   /* Nano RX <- E32 TX (SoftwareSerial) */
#define LORA_TX_PIN  3   /* Nano TX -> E32 RX (SoftwareSerial) */
#define LORA_AUX_PIN 6   /* wired for future use; not read by this sketch */

#define LORA_SYNC0  0xAA
#define LORA_SYNC1  0x55

/* Same enum as the flight computer, for decoding flightPhase to a name */
enum FlightPhase { PRELAUNCH = 0, ASCENT = 1, NEAR_APOGEE = 2, DESCENT = 3, LANDED = 4 };
const char *PHASE_NAMES[5] = { "PRELAUNCH", "ASCENT", "NEAR_APOGEE", "DESCENT", "LANDED" };

#define ALT_SRC_NONE 0
#define ALT_SRC_BMP  1
#define ALT_SRC_GPS  2
const char *ALT_SRC_NAMES[3] = { "NONE", "BMP", "GPS" };

SoftwareSerial loraSerial(LORA_RX_PIN, LORA_TX_PIN);

/* =========================================================================
   TelemetryFrame — MUST match the transmitter exactly
   ========================================================================= */
#pragma pack(push, 1)
struct TelemetryFrame {
  uint8_t  sync0, sync1;
  uint32_t millis_ms;
  int16_t  bmpTempC_x100;
  int32_t  bmpPress_Pa;
  int16_t  bmpAlt_m_x10;
  uint8_t  bmpFlags;              /* bit0=ok bit1=valid bit2=disabled */
  int16_t  ax_x1000, ay_x1000, az_x1000;
  uint16_t accelMag_x1000;
  int16_t  ahtTempC_x100;
  uint16_t ahtHum_x100;
  int16_t  dewPointC_x100;
  int16_t  dsInC_x100, dsOutC_x100;
  int16_t  tempDiffC_x100;
  int16_t  tempGrad_x1000;
  uint16_t uvIndex_x100;
  uint16_t battery_x100;
  uint8_t  flightPhase;
  int16_t  maxAlt_m_x10;
  uint8_t  healthMask;
  int32_t  gpsLat_x1e6;
  int32_t  gpsLon_x1e6;
  int16_t  gpsAlt_m_x10;
  uint16_t gpsSpeedKmh_x10;
  uint8_t  gpsSatellites;
  uint8_t  gpsFlags;               /* bit0=fixValid bit1=hasAltitude bit2=stale */
  uint8_t  checksum;
};
#pragma pack(pop)
#define FRAME_SIZE sizeof(TelemetryFrame)

/* =========================================================================
   Frame receive state machine
   ========================================================================= */
enum RxState { WAIT_SYNC0, WAIT_SYNC1, READING };
RxState rxState = WAIT_SYNC0;
uint8_t frameBuf[FRAME_SIZE];
uint8_t rxIndex = 0;
unsigned long lastByteMillis = 0;
#define FRAME_TIMEOUT_MS 500  /* abandon a partial frame if it stalls this long */

unsigned long framesOk = 0;
unsigned long framesBadChecksum = 0;
unsigned long framesTimedOut = 0;

/* =========================================================================
   Checksum — identical algorithm to the transmitter: XOR of every byte
   between the sync bytes and the checksum byte itself.
   ========================================================================= */
uint8_t ComputeChecksum(const uint8_t *data, size_t len)
{
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) sum ^= data[i];
  return sum;
}

/* =========================================================================
   Decode + print one validated frame as a CSV row.
   Column order mirrors the flight computer's own SD log for easy
   side-by-side comparison / import into the same spreadsheet.
   ========================================================================= */
void PrintFrameCsv(const TelemetryFrame &f)
{
  Serial.print(f.millis_ms); Serial.print(',');

  Serial.print(f.bmpTempC_x100 / 100.0f, 2); Serial.print(',');
  Serial.print(f.bmpPress_Pa); Serial.print(',');
  Serial.print(f.bmpAlt_m_x10 / 10.0f, 1); Serial.print(',');
  Serial.print((f.bmpFlags & 0x01) ? 1 : 0); Serial.print(',');  /* bmp ok */
  Serial.print((f.bmpFlags & 0x02) ? 1 : 0); Serial.print(',');  /* bmp altitude valid */
  Serial.print((f.bmpFlags & 0x04) ? 1 : 0); Serial.print(',');  /* bmp altitude disabled */

  Serial.print(f.ax_x1000 / 1000.0f, 3); Serial.print(',');
  Serial.print(f.ay_x1000 / 1000.0f, 3); Serial.print(',');
  Serial.print(f.az_x1000 / 1000.0f, 3); Serial.print(',');
  Serial.print(f.accelMag_x1000 / 1000.0f, 3); Serial.print(',');

  Serial.print(f.ahtTempC_x100 / 100.0f, 2); Serial.print(',');
  Serial.print(f.ahtHum_x100 / 100.0f, 2); Serial.print(',');
  Serial.print(f.dewPointC_x100 / 100.0f, 2); Serial.print(',');

  Serial.print(f.dsInC_x100 / 100.0f, 2); Serial.print(',');
  Serial.print(f.dsOutC_x100 / 100.0f, 2); Serial.print(',');
  Serial.print(f.tempDiffC_x100 / 100.0f, 2); Serial.print(',');
  Serial.print(f.tempGrad_x1000 / 1000.0f, 4); Serial.print(',');

  Serial.print(f.uvIndex_x100 / 100.0f, 2); Serial.print(',');
  Serial.print(f.battery_x100 / 100.0f, 2); Serial.print(',');

  Serial.print(PHASE_NAMES[f.flightPhase]); Serial.print(',');
  Serial.print(f.maxAlt_m_x10 / 10.0f, 1); Serial.print(',');
  Serial.print(F("0x")); Serial.print(f.healthMask, HEX); Serial.print(',');

  Serial.print(f.gpsLat_x1e6 / 1000000.0f, 6); Serial.print(',');
  Serial.print(f.gpsLon_x1e6 / 1000000.0f, 6); Serial.print(',');
  Serial.print(f.gpsAlt_m_x10 / 10.0f, 1); Serial.print(',');
  Serial.print(f.gpsSpeedKmh_x10 / 10.0f, 1); Serial.print(',');
  Serial.print(f.gpsSatellites); Serial.print(',');
  Serial.print((f.gpsFlags & 0x01) ? 1 : 0); Serial.print(',');  /* gps fix valid */
  Serial.print((f.gpsFlags & 0x02) ? 1 : 0); Serial.print(',');  /* gps has altitude */
  Serial.print((f.gpsFlags & 0x04) ? 1 : 0); Serial.print(',');  /* gps stale */

  Serial.print(framesOk); Serial.print(',');
  Serial.println(framesBadChecksum);
}

void PrintCsvHeader()
{
  Serial.println(F(
    "tx_millis,bmp_tempC,bmp_pressPa,bmp_altitude_m,bmp_ok,bmp_altitude_valid,bmp_altitude_disabled,"
    "accel_ax_g,accel_ay_g,accel_az_g,accel_mag_g,"
    "aht_tempC,aht_humPct,dew_point_C,"
    "ds_in_tempC,ds_out_tempC,temp_diff_C,temp_gradient_Cps,"
    "uv_index,battery_voltage,"
    "flight_phase,max_altitude_m,sensor_health_hex,"
    "gps_lat,gps_lon,gps_alt_m,gps_speed_kmh,gps_satellites,gps_fix_valid,gps_has_altitude,gps_stale,"
    "rx_frames_ok,rx_frames_bad_checksum"
  ));
}

/* =========================================================================
   Feed the receive state machine one byte at a time from the E32 link.
   Called every loop() iteration so nothing is missed between frames.
   ========================================================================= */
void LoRa_ReceiveUpdate()
{
  /* Abandon a stalled partial frame so a single dropped byte can't wedge
     the parser forever waiting for bytes that will never arrive. */
  if (rxState != WAIT_SYNC0 && (millis() - lastByteMillis > FRAME_TIMEOUT_MS))
  {
    rxState = WAIT_SYNC0;
    rxIndex = 0;
    framesTimedOut++;
  }

  while (loraSerial.available())
  {
    uint8_t b = loraSerial.read();
    lastByteMillis = millis();

    switch (rxState)
    {
      case WAIT_SYNC0:
        if (b == LORA_SYNC0)
        {
          frameBuf[0] = b;
          rxIndex = 1;
          rxState = WAIT_SYNC1;
        }
        break;

      case WAIT_SYNC1:
        if (b == LORA_SYNC1)
        {
          frameBuf[1] = b;
          rxIndex = 2;
          rxState = READING;
        }
        else if (b == LORA_SYNC0)
        {
          /* stay resynced in case sync0 repeats before sync1 arrives */
          frameBuf[0] = b;
          rxIndex = 1;
        }
        else
        {
          rxState = WAIT_SYNC0;
          rxIndex = 0;
        }
        break;

      case READING:
        frameBuf[rxIndex++] = b;
        if (rxIndex >= FRAME_SIZE)
        {
          /* Full frame collected — validate and decode */
          uint8_t expectedChecksum = ComputeChecksum(frameBuf + 2, FRAME_SIZE - 3);
          uint8_t receivedChecksum = frameBuf[FRAME_SIZE - 1];

          if (expectedChecksum == receivedChecksum)
          {
            framesOk++;
            TelemetryFrame f;
            memcpy(&f, frameBuf, FRAME_SIZE);
            PrintFrameCsv(f);
          }
          else
          {
            framesBadChecksum++;
          }

          rxState = WAIT_SYNC0;
          rxIndex = 0;
        }
        break;
    }
  }
}

void setup()
{
  Serial.begin(PROJECT_BAUD);        /* to PC */
  loraSerial.begin(LORA_UART_BAUD);  /* to E32 */
  pinMode(LORA_AUX_PIN, INPUT);

  Serial.println(F("===== HAB01 Ground Station Receiver ====="));
  PrintCsvHeader();

  lastByteMillis = millis();
}

void loop()
{
  LoRa_ReceiveUpdate();
}
