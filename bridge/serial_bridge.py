import argparse
import csv
import datetime
import json
import logging
import math
import os
import random
import sys
import time
from typing import Dict, Any, Optional

import requests
import serial
from serial.tools import list_ports

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

HEADER_ROW = [
    "tx_millis", "bmp_tempC", "bmp_pressPa", "bmp_altitude_m", "bmp_ok", "bmp_altitude_valid",
    "bmp_altitude_disabled", "accel_ax_g", "accel_ay_g", "accel_az_g", "accel_mag_g", "aht_tempC",
    "aht_humPct", "dew_point_C", "ds_in_tempC", "ds_out_tempC", "temp_diff_C", "temp_gradient_Cps",
    "uv_index", "battery_voltage", "flight_phase", "max_altitude_m", "sensor_health_hex", "gps_lat",
    "gps_lon", "gps_alt_m", "gps_speed_kmh", "gps_satellites", "gps_fix_valid", "gps_has_altitude",
    "gps_stale", "rx_frames_ok", "rx_frames_bad_checksum"
]

def auto_detect_port() -> Optional[str]:
    ports = list(list_ports.comports())
    for p in ports:
        if "Arduino" in p.description or "CH340" in p.description or "USB Serial" in p.description:
            return p.device
    if ports:
        return ports[0].device
    return None

def parse_line(line: str) -> Optional[Dict[str, Any]]:
    line = line.strip()
    if not line or line.startswith("=====") or line.startswith("tx_millis"):
        return None
        
    parts = line.split(',')
    if len(parts) != len(HEADER_ROW):
        logging.warning(f"Skipping malformed line (expected {len(HEADER_ROW)} fields, got {len(parts)}): {line}")
        return None
        
    try:
        data = {
            "tx_millis": int(parts[0]),
            "bmp_tempC": float(parts[1]),
            "bmp_pressPa": float(parts[2]),
            "bmp_altitude_m": float(parts[3]),
            "bmp_ok": bool(int(parts[4])),
            "bmp_altitude_valid": bool(int(parts[5])),
            "bmp_altitude_disabled": bool(int(parts[6])),
            "accel_ax_g": float(parts[7]),
            "accel_ay_g": float(parts[8]),
            "accel_az_g": float(parts[9]),
            "accel_mag_g": float(parts[10]),
            "aht_tempC": float(parts[11]),
            "aht_humPct": float(parts[12]),
            "dew_point_C": float(parts[13]),
            "ds_in_tempC": float(parts[14]),
            "ds_out_tempC": float(parts[15]),
            "temp_diff_C": float(parts[16]),
            "temp_gradient_Cps": float(parts[17]),
            "uv_index": float(parts[18]),
            "battery_voltage": float(parts[19]),
            "flight_phase": parts[20],
            "max_altitude_m": float(parts[21]),
            "sensor_health_hex": parts[22],
            "gps_lat": float(parts[23]),
            "gps_lon": float(parts[24]),
            "gps_alt_m": float(parts[25]),
            "gps_speed_kmh": float(parts[26]),
            "gps_satellites": int(parts[27]),
            "gps_fix_valid": bool(int(parts[28])),
            "gps_has_altitude": bool(int(parts[29])),
            "gps_stale": bool(int(parts[30])),
            "rx_frames_ok": int(parts[31]),
            "rx_frames_bad_checksum": int(parts[32])
        }
        data["received_at"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        return data
    except Exception as e:
        logging.error(f"Error parsing line: {line}. Error: {e}")
        return None

def send_data(api_url: str, data: Dict[str, Any]):
    try:
        res = requests.post(f"{api_url}/ingest", json=data, timeout=2.0)
        res.raise_for_status()
        logging.info(f"Posted frame tx_millis={data.get('tx_millis')}")
    except requests.exceptions.RequestException as e:
        logging.error(f"Failed to post data to {api_url}: {e}")

def simulate_data() -> str:
    # Basic simulation for --simulate mode
    tx_millis = int(time.time() * 1000) % 10000000
    return f"{tx_millis},25.0,101325,0,1,1,0,0,0,1,1,24.0,50.0,15.0,22.0,20.0,2.0,0.1,1.0,8.2,PRELAUNCH,0,0x7F,37.7749,-122.4194,10,0,8,1,1,0,10,0"

def run_bridge(port: str, baud: int, api_url: str, simulate: bool):
    if simulate:
        logging.info("Running in SIMULATE mode")
        while True:
            line = simulate_data()
            data = parse_line(line)
            if data:
                send_data(api_url, data)
            time.sleep(2)
            
    backoff = 1
    while True:
        try:
            if not port:
                port = auto_detect_port()
            
            if not port:
                logging.warning("No serial port found, retrying...")
                time.sleep(backoff)
                backoff = min(30, backoff * 2)
                continue
                
            logging.info(f"Connecting to {port} at {baud} baud")
            with serial.Serial(port, baud, timeout=1.0) as ser:
                logging.info(f"Connected to {port}")
                backoff = 1
                while True:
                    line = ser.readline().decode('utf-8', errors='replace')
                    if line:
                        data = parse_line(line)
                        if data:
                            send_data(api_url, data)
        except serial.SerialException as e:
            logging.error(f"Serial error: {e}")
            logging.info(f"Reconnecting in {backoff} seconds...")
            port = None # Force re-detect if needed
            time.sleep(backoff)
            backoff = min(30, backoff * 2)
        except Exception as e:
            logging.error(f"Unexpected error: {e}")
            time.sleep(backoff)
            backoff = min(30, backoff * 2)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Serial Bridge for HAB Dashboard")
    parser.add_argument("--port", type=str, default=os.environ.get("SERIAL_PORT"), help="Serial port to use")
    parser.add_argument("--baud", type=int, default=int(os.environ.get("BAUD_RATE", 9600)), help="Baud rate")
    parser.add_argument("--api-url", type=str, default=os.environ.get("API_URL", "http://localhost:8000"), help="Backend API URL")
    parser.add_argument("--simulate", action="store_true", help="Run in simulation mode")
    
    args = parser.parse_args()
    
    run_bridge(args.port, args.baud, args.api_url, args.simulate)
