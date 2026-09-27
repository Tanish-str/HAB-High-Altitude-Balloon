import sqlite3
import threading
import os
from contextlib import contextmanager
from typing import Optional, List, Dict, Any
from config import settings

# Global lock for write operations to ensure thread-safety
db_write_lock = threading.Lock()

def init_db():
    os.makedirs(os.path.dirname(settings.DATABASE_PATH), exist_ok=True)
    conn = sqlite3.connect(settings.DATABASE_PATH)
    cursor = conn.cursor()
    
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS flights (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        started_at TEXT NOT NULL,
        ended_at TEXT,
        notes TEXT
    )
    ''')
    
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS telemetry_frames (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        flight_id INTEGER NOT NULL REFERENCES flights(id),
        received_at TEXT NOT NULL,
        tx_millis INTEGER,
        bmp_tempC REAL,
        bmp_pressPa REAL,
        bmp_altitude_m REAL,
        bmp_ok INTEGER,
        bmp_altitude_valid INTEGER,
        bmp_altitude_disabled INTEGER,
        accel_ax_g REAL,
        accel_ay_g REAL,
        accel_az_g REAL,
        accel_mag_g REAL,
        aht_tempC REAL,
        aht_humPct REAL,
        dew_point_C REAL,
        ds_in_tempC REAL,
        ds_out_tempC REAL,
        temp_diff_C REAL,
        temp_gradient_Cps REAL,
        uv_index REAL,
        battery_voltage REAL,
        flight_phase TEXT,
        max_altitude_m REAL,
        sensor_health_hex TEXT,
        gps_lat REAL,
        gps_lon REAL,
        gps_alt_m REAL,
        gps_speed_kmh REAL,
        gps_satellites INTEGER,
        gps_fix_valid INTEGER,
        gps_has_altitude INTEGER,
        gps_stale INTEGER,
        rx_frames_ok INTEGER,
        rx_frames_bad_checksum INTEGER
    )
    ''')
    
    cursor.execute('CREATE INDEX IF NOT EXISTS idx_frames_flight_time ON telemetry_frames(flight_id, received_at)')
    conn.commit()
    conn.close()

def get_db():
    conn = sqlite3.connect(settings.DATABASE_PATH, check_same_thread=False)
    conn.row_factory = sqlite3.Row
    try:
        yield conn
    finally:
        conn.close()

def insert_frame(db: sqlite3.Connection, flight_id: int, frame_dict: dict) -> dict:
    fields = ['flight_id']
    values = [flight_id]
    
    for key, value in frame_dict.items():
        if key not in ['flight_id', 'id']:
            fields.append(key)
            values.append(value)
            
    placeholders = ', '.join(['?'] * len(values))
    columns = ', '.join(fields)
    
    with db_write_lock:
        cursor = db.cursor()
        cursor.execute(f"INSERT INTO telemetry_frames ({columns}) VALUES ({placeholders})", values)
        db.commit()
        inserted_id = cursor.lastrowid
        
        cursor.execute("SELECT * FROM telemetry_frames WHERE id = ?", (inserted_id,))
        return dict(cursor.fetchone())

def get_frames(db: sqlite3.Connection, flight_id: int, since: Optional[str] = None, until: Optional[str] = None, limit: int = 1000, offset: int = 0, order: str = 'asc') -> List[dict]:
    query = "SELECT * FROM telemetry_frames WHERE flight_id = ?"
    params = [flight_id]
    
    if since:
        query += " AND received_at >= ?"
        params.append(since)
    if until:
        query += " AND received_at <= ?"
        params.append(until)
        
    order_clause = "ASC" if order.lower() == 'asc' else "DESC"
    query += f" ORDER BY received_at {order_clause} LIMIT ? OFFSET ?"
    params.extend([limit, offset])
    
    cursor = db.cursor()
    cursor.execute(query, params)
    return [dict(row) for row in cursor.fetchall()]

def get_latest_frame(db: sqlite3.Connection, flight_id: int) -> Optional[dict]:
    cursor = db.cursor()
    cursor.execute("SELECT * FROM telemetry_frames WHERE flight_id = ? ORDER BY received_at DESC LIMIT 1", (flight_id,))
    row = cursor.fetchone()
    return dict(row) if row else None

def create_flight(db: sqlite3.Connection, name: str) -> dict:
    import datetime
    now_str = datetime.datetime.utcnow().isoformat()
    
    with db_write_lock:
        cursor = db.cursor()
        cursor.execute("INSERT INTO flights (name, started_at) VALUES (?, ?)", (name, now_str))
        db.commit()
        inserted_id = cursor.lastrowid
        
    return get_flight(db, inserted_id)

def get_flights(db: sqlite3.Connection) -> List[dict]:
    cursor = db.cursor()
    cursor.execute("""
        SELECT f.*, COUNT(t.id) as frame_count, 
               (SELECT flight_phase FROM telemetry_frames WHERE flight_id = f.id ORDER BY received_at DESC LIMIT 1) as latest_phase
        FROM flights f
        LEFT JOIN telemetry_frames t ON f.id = t.flight_id
        GROUP BY f.id
        ORDER BY f.started_at DESC
    """)
    return [dict(row) for row in cursor.fetchall()]

def get_flight(db: sqlite3.Connection, flight_id: int) -> Optional[dict]:
    cursor = db.cursor()
    cursor.execute("""
        SELECT f.*, COUNT(t.id) as frame_count,
               (SELECT flight_phase FROM telemetry_frames WHERE flight_id = f.id ORDER BY received_at DESC LIMIT 1) as latest_phase
        FROM flights f
        LEFT JOIN telemetry_frames t ON f.id = t.flight_id
        WHERE f.id = ?
        GROUP BY f.id
    """, (flight_id,))
    row = cursor.fetchone()
    return dict(row) if row else None

def update_flight(db: sqlite3.Connection, flight_id: int, updates: dict) -> Optional[dict]:
    if not updates:
        return get_flight(db, flight_id)
        
    set_clauses = []
    params = []
    for key, value in updates.items():
        set_clauses.append(f"{key} = ?")
        params.append(value)
        
    params.append(flight_id)
    
    with db_write_lock:
        cursor = db.cursor()
        cursor.execute(f"UPDATE flights SET {', '.join(set_clauses)} WHERE id = ?", params)
        db.commit()
        
    return get_flight(db, flight_id)

def get_active_flight(db: sqlite3.Connection) -> Optional[dict]:
    cursor = db.cursor()
    cursor.execute("""
        SELECT f.*, COUNT(t.id) as frame_count,
               (SELECT flight_phase FROM telemetry_frames WHERE flight_id = f.id ORDER BY received_at DESC LIMIT 1) as latest_phase
        FROM flights f
        LEFT JOIN telemetry_frames t ON f.id = t.flight_id
        WHERE f.ended_at IS NULL
        GROUP BY f.id
        ORDER BY f.started_at DESC LIMIT 1
    """)
    row = cursor.fetchone()
    return dict(row) if row else None
