from fastapi import FastAPI, Depends, HTTPException, Query, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import Response
from typing import List, Optional
import sqlite3
import datetime
import json
import asyncio
from contextlib import asynccontextmanager
import time

from database import (
    init_db, get_db, insert_frame, get_frames, get_latest_frame,
    create_flight, get_flights, get_flight, update_flight, get_active_flight
)
from models import (
    TelemetryFrameIn, TelemetryFrameOut, FlightCreate, FlightUpdate,
    FlightOut, HealthResponse
)

# App State for WebSockets and Health
class AppState:
    def __init__(self):
        self.start_time = time.time()
        self.last_frame_time: Optional[float] = None
        self.last_frame_received_at: Optional[str] = None
        self.last_rx_frames_bad_checksum: Optional[int] = None
        self.active_connections: List[WebSocket] = []

app_state = AppState()

@asynccontextmanager
async def lifespan(app: FastAPI):
    init_db()
    yield
    # Cleanup on shutdown if needed

app = FastAPI(title="HAB Ground Station API", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# WebSocket Manager
class ConnectionManager:
    def __init__(self):
        self.active_connections: List[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)

    async def broadcast(self, message: dict):
        text_message = json.dumps(message)
        dead_connections = []
        for connection in self.active_connections:
            try:
                await connection.send_text(text_message)
            except Exception:
                dead_connections.append(connection)
        
        for dead_conn in dead_connections:
            self.disconnect(dead_conn)

manager = ConnectionManager()


@app.get("/health", response_model=HealthResponse)
def health_check():
    now = time.time()
    uptime = now - app_state.start_time
    
    # Consider bridge connected if we received a frame in the last 15 seconds
    bridge_connected = False
    if app_state.last_frame_time and (now - app_state.last_frame_time) < 15.0:
        bridge_connected = True
        
    return HealthResponse(
        status="ok",
        uptime_seconds=uptime,
        last_frame_received_at=app_state.last_frame_received_at,
        rx_frames_bad_checksum=app_state.last_rx_frames_bad_checksum,
        bridge_connected=bridge_connected
    )


@app.get("/flights", response_model=List[FlightOut])
def list_flights(db: sqlite3.Connection = Depends(get_db)):
    return get_flights(db)


@app.get("/flights/{flight_id}", response_model=FlightOut)
def get_flight_endpoint(flight_id: int, db: sqlite3.Connection = Depends(get_db)):
    flight = get_flight(db, flight_id)
    if not flight:
        raise HTTPException(status_code=404, detail="Flight not found")
    return flight


@app.post("/flights", response_model=FlightOut)
async def create_flight_endpoint(flight: FlightCreate, db: sqlite3.Connection = Depends(get_db)):
    created = create_flight(db, flight.name)
    await manager.broadcast({"type": "flight_update", "flight": created})
    return created


@app.patch("/flights/{flight_id}", response_model=FlightOut)
async def update_flight_endpoint(flight_id: int, flight: FlightUpdate, db: sqlite3.Connection = Depends(get_db)):
    updates = flight.model_dump(exclude_unset=True)
    updated = update_flight(db, flight_id, updates)
    if not updated:
        raise HTTPException(status_code=404, detail="Flight not found")
    await manager.broadcast({"type": "flight_update", "flight": updated})
    return updated


@app.get("/flights/{flight_id}/frames", response_model=List[TelemetryFrameOut])
def get_flight_frames(
    flight_id: int,
    since: Optional[str] = None,
    until: Optional[str] = None,
    limit: int = 1000,
    offset: int = 0,
    order: str = Query('asc', pattern='^(asc|desc)$'),
    db: sqlite3.Connection = Depends(get_db)
):
    # Verify flight exists
    if not get_flight(db, flight_id):
        raise HTTPException(status_code=404, detail="Flight not found")
    return get_frames(db, flight_id, since, until, limit, offset, order)


@app.get("/flights/{flight_id}/frames/latest", response_model=Optional[TelemetryFrameOut])
def get_flight_latest_frame(flight_id: int, db: sqlite3.Connection = Depends(get_db)):
    if not get_flight(db, flight_id):
        raise HTTPException(status_code=404, detail="Flight not found")
    frame = get_latest_frame(db, flight_id)
    return frame if frame else Response(status_code=204)


@app.get("/flights/{flight_id}/export")
def export_flight_frames(flight_id: int, db: sqlite3.Connection = Depends(get_db)):
    flight = get_flight(db, flight_id)
    if not flight:
        raise HTTPException(status_code=404, detail="Flight not found")
        
    frames = get_frames(db, flight_id, limit=1000000, order='asc')
    if not frames:
        return Response(content="", media_type="text/csv")
        
    import csv
    import io
    
    output = io.StringIO()
    writer = csv.DictWriter(output, fieldnames=frames[0].keys())
    writer.writeheader()
    for frame in frames:
        writer.writerow(frame)
        
    return Response(
        content=output.getvalue(),
        media_type="text/csv",
        headers={"Content-Disposition": f"attachment; filename=flight_{flight_id}_export.csv"}
    )


@app.post("/ingest", response_model=TelemetryFrameOut)
async def ingest_frame(frame_in: TelemetryFrameIn, db: sqlite3.Connection = Depends(get_db)):
    app_state.last_frame_time = time.time()
    app_state.last_frame_received_at = frame_in.received_at
    if frame_in.rx_frames_bad_checksum is not None:
        app_state.last_rx_frames_bad_checksum = frame_in.rx_frames_bad_checksum
        
    # Detect sentinel values (<= -300 for floats)
    frame_dict = frame_in.model_dump(exclude_unset=True)
    for key, value in frame_dict.items():
        if isinstance(value, float) and value <= -300.0:
            frame_dict[key] = None
            
    # Check for active flight
    active_flight = get_active_flight(db)
    if not active_flight:
        date_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
        active_flight = create_flight(db, f"Flight - {date_str}")
        await manager.broadcast({"type": "flight_update", "flight": active_flight})
        
    # Insert frame
    inserted = insert_frame(db, active_flight['id'], frame_dict)
    
    # Broadcast to websocket
    await manager.broadcast({
        "type": "frame",
        "flight_id": active_flight['id'],
        "data": inserted
    })
    
    return inserted


@app.websocket("/live")
async def websocket_endpoint(websocket: WebSocket):
    await manager.connect(websocket)
    try:
        while True:
            # Keep connection alive, wait for client messages if any
            _ = await websocket.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(websocket)
