import argparse
import datetime
import math
import time
import requests
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def get_flight_phase(t: float) -> str:
    if t < 30:
        return "PRELAUNCH"
    elif t < 150:
        return "ASCENT"
    elif t < 180:
        return "NEAR_APOGEE"
    elif t < 270:
        return "DESCENT"
    else:
        return "LANDED"

def generate_telemetry(t: float, max_t: float = 300.0) -> dict:
    phase = get_flight_phase(t)
    
    # Base values
    alt = 0.0
    lat = 37.7749
    lon = -122.4194
    
    if phase == "PRELAUNCH":
        alt = random_noise(10, 2)
    elif phase == "ASCENT":
        # Climb to 30000m over 120s
        progress = (t - 30) / 120.0
        alt = 10 + (30000 * progress) + random_noise(0, 50)
        lat += progress * 0.1
        lon += progress * 0.1
    elif phase == "NEAR_APOGEE":
        alt = 30010 + random_noise(0, 20)
        lat += 0.1
        lon += 0.1
    elif phase == "DESCENT":
        # Drop from 30000m to 0m over 90s
        progress = (t - 180) / 90.0
        alt = 30000 - (30000 * progress) + random_noise(0, 50)
        lat += 0.1 + progress * 0.05
        lon += 0.1 + progress * 0.05
    elif phase == "LANDED":
        alt = random_noise(5, 2)
        lat += 0.15
        lon += 0.15

    alt = max(0, alt)
    
    # Physics approx
    temp = 25.0 - (alt / 150.0)  # simple lapse rate
    press = 101325 * math.exp(-alt / 8400.0)
    
    return {
        "tx_millis": int(t * 1000),
        "bmp_tempC": temp + random_noise(0, 0.5),
        "bmp_pressPa": press + random_noise(0, 10),
        "bmp_altitude_m": alt,
        "bmp_ok": True,
        "bmp_altitude_valid": True,
        "bmp_altitude_disabled": False,
        "accel_ax_g": 0.0 + random_noise(0, 0.1),
        "accel_ay_g": 0.0 + random_noise(0, 0.1),
        "accel_az_g": 1.0 + (0.5 if phase in ["ASCENT", "DESCENT"] else 0) + random_noise(0, 0.1),
        "accel_mag_g": 1.0,
        "aht_tempC": temp - 1 + random_noise(0, 0.5),
        "aht_humPct": 50.0 - (alt / 1000.0) + random_noise(0, 1),
        "dew_point_C": temp - 10,
        "ds_in_tempC": max(10, temp + 15),
        "ds_out_tempC": temp,
        "temp_diff_C": 15.0,
        "temp_gradient_Cps": 0.1,
        "uv_index": min(10, alt / 3000.0),
        "battery_voltage": max(7.0, 8.4 - (t / 300.0)),
        "flight_phase": phase,
        "max_altitude_m": 30000 if phase in ["DESCENT", "LANDED"] else alt,
        "sensor_health_hex": "0x7F",
        "gps_lat": lat,
        "gps_lon": lon,
        "gps_alt_m": alt,
        "gps_speed_kmh": 20.0 if phase in ["ASCENT", "DESCENT"] else 0.0,
        "gps_satellites": 8,
        "gps_fix_valid": True,
        "gps_has_altitude": True,
        "gps_stale": False,
        "rx_frames_ok": int(t / 2),
        "rx_frames_bad_checksum": 0,
        "received_at": datetime.datetime.now(datetime.timezone.utc).isoformat()
    }

def random_noise(base: float, jitter: float) -> float:
    import random
    return base + random.uniform(-jitter, jitter)

def simulate_flight(api_url: str):
    logging.info(f"Starting 5-minute simulated flight, sending to {api_url}")
    t = 0.0
    while t <= 300.0:
        data = generate_telemetry(t)
        try:
            res = requests.post(f"{api_url}/ingest", json=data, timeout=2.0)
            res.raise_for_status()
            logging.info(f"[{data['flight_phase']}] alt={data['bmp_altitude_m']:.1f}m - OK")
        except Exception as e:
            logging.error(f"Failed to send data: {e}")
        
        time.sleep(2.0)
        t += 2.0
    
    logging.info("Simulation complete.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Simulate HAB flight telemetry")
    parser.add_argument("--api-url", type=str, default="http://localhost:8000", help="Backend API URL")
    args = parser.parse_args()
    simulate_flight(args.api_url)
