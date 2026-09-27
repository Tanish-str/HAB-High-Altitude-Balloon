export interface TelemetryFrame {
  id: number;
  flight_id: number;
  received_at: string;
  tx_millis: number;
  bmp_tempC: number | null;
  bmp_pressPa: number | null;
  bmp_altitude_m: number | null;
  bmp_ok: boolean;
  bmp_altitude_valid: boolean;
  bmp_altitude_disabled: boolean;
  accel_ax_g: number | null;
  accel_ay_g: number | null;
  accel_az_g: number | null;
  accel_mag_g: number | null;
  aht_tempC: number | null;
  aht_humPct: number | null;
  dew_point_C: number | null;
  ds_in_tempC: number | null;
  ds_out_tempC: number | null;
  temp_diff_C: number | null;
  temp_gradient_Cps: number | null;
  uv_index: number | null;
  battery_voltage: number | null;
  flight_phase: 'PRELAUNCH' | 'ASCENT' | 'NEAR_APOGEE' | 'DESCENT' | 'LANDED';
  max_altitude_m: number | null;
  sensor_health_hex: string;
  gps_lat: number | null;
  gps_lon: number | null;
  gps_alt_m: number | null;
  gps_speed_kmh: number | null;
  gps_satellites: number;
  gps_fix_valid: boolean;
  gps_has_altitude: boolean;
  gps_stale: boolean;
  rx_frames_ok: number;
  rx_frames_bad_checksum: number;
}

export interface Flight {
  id: number;
  name: string;
  started_at: string;
  ended_at: string | null;
  notes: string | null;
  frame_count: number;
  latest_phase?: string;
}

export interface HealthStatus {
  serial_connected: boolean;
  last_frame_at: string | null;
  rx_frames_bad_checksum: number;
  uptime_seconds: number;
}

export type FlightPhase = 'PRELAUNCH' | 'ASCENT' | 'NEAR_APOGEE' | 'DESCENT' | 'LANDED';

export const SENSOR_HEALTH_BITS: Record<number, { name: string; label: string }> = {
  0: { name: 'BMP180', label: 'Pressure' },
  1: { name: 'MPU6050', label: 'Accelerometer' },
  2: { name: 'AHT10', label: 'Humidity' },
  3: { name: 'DS18B20_IN', label: 'Temp (Inside)' },
  4: { name: 'DS18B20_OUT', label: 'Temp (Outside)' },
  5: { name: 'BMP_ALT', label: 'Altitude Trust' },
  6: { name: 'GPS', label: 'GPS Fix' },
  7: { name: 'SD_WRITE', label: 'SD Card' },
};

export function parseSensorHealth(hex: string): Record<string, boolean> {
  const value = parseInt(hex, 16);
  const result: Record<string, boolean> = {};
  for (const [bit, info] of Object.entries(SENSOR_HEALTH_BITS)) {
    result[info.name] = (value & (1 << Number(bit))) !== 0;
  }
  return result;
}
