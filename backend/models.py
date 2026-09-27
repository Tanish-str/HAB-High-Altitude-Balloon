from typing import Optional, Dict, Any, List
from pydantic import BaseModel, ConfigDict
from datetime import datetime

class TelemetryFrameIn(BaseModel):
    received_at: str
    tx_millis: Optional[int] = None
    bmp_tempC: Optional[float] = None
    bmp_pressPa: Optional[float] = None
    bmp_altitude_m: Optional[float] = None
    bmp_ok: Optional[int] = None
    bmp_altitude_valid: Optional[int] = None
    bmp_altitude_disabled: Optional[int] = None
    accel_ax_g: Optional[float] = None
    accel_ay_g: Optional[float] = None
    accel_az_g: Optional[float] = None
    accel_mag_g: Optional[float] = None
    aht_tempC: Optional[float] = None
    aht_humPct: Optional[float] = None
    dew_point_C: Optional[float] = None
    ds_in_tempC: Optional[float] = None
    ds_out_tempC: Optional[float] = None
    temp_diff_C: Optional[float] = None
    temp_gradient_Cps: Optional[float] = None
    uv_index: Optional[float] = None
    battery_voltage: Optional[float] = None
    flight_phase: Optional[str] = None
    max_altitude_m: Optional[float] = None
    sensor_health_hex: Optional[str] = None
    gps_lat: Optional[float] = None
    gps_lon: Optional[float] = None
    gps_alt_m: Optional[float] = None
    gps_speed_kmh: Optional[float] = None
    gps_satellites: Optional[int] = None
    gps_fix_valid: Optional[int] = None
    gps_has_altitude: Optional[int] = None
    gps_stale: Optional[int] = None
    rx_frames_ok: Optional[int] = None
    rx_frames_bad_checksum: Optional[int] = None
    
    model_config = ConfigDict(extra='allow')

class TelemetryFrameOut(TelemetryFrameIn):
    id: int
    flight_id: int

class FlightCreate(BaseModel):
    name: str

class FlightUpdate(BaseModel):
    name: Optional[str] = None
    notes: Optional[str] = None
    ended_at: Optional[str] = None

class FlightOut(BaseModel):
    id: int
    name: str
    started_at: str
    ended_at: Optional[str] = None
    notes: Optional[str] = None
    frame_count: int
    latest_phase: Optional[str] = None

class HealthResponse(BaseModel):
    status: str
    uptime_seconds: float
    last_frame_received_at: Optional[str] = None
    rx_frames_bad_checksum: Optional[int] = None
    bridge_connected: bool
