import React, { useEffect, useState } from 'react';
import { MapContainer, TileLayer, Marker, Polyline, Popup } from 'react-leaflet';
import { useWebSocket } from '../contexts/WebSocketContext';

export function FlightMap() {
  const { frames, latestFrame } = useWebSocket();
  const [position, setPosition] = useState<[number, number]>([0, 0]);

  // Extract path from valid GPS fixes
  const path = frames
    .filter(f => f.gps_fix_valid && f.gps_lat !== null && f.gps_lon !== null)
    .map(f => [f.gps_lat!, f.gps_lon!] as [number, number]);

  useEffect(() => {
    if (latestFrame?.gps_fix_valid && latestFrame.gps_lat && latestFrame.gps_lon) {
      setPosition([latestFrame.gps_lat, latestFrame.gps_lon]);
    }
  }, [latestFrame]);

  if (!latestFrame?.gps_fix_valid && path.length === 0) {
    return (
      <div className="w-full h-full bg-[#1e2130] flex items-center justify-center border border-[#2a2d3a] rounded-lg">
        <div className="text-center">
          <div className="text-danger text-xl mb-2">No GPS Fix</div>
          <div className="text-muted">Waiting for valid coordinates...</div>
        </div>
      </div>
    );
  }

  // Use the latest valid position or default
  const center = path.length > 0 ? path[path.length - 1] : position;

  return (
    <div className="w-full h-full border border-[#2a2d3a] rounded-lg overflow-hidden relative">
      <MapContainer center={center} zoom={13} style={{ height: '100%', width: '100%' }}>
        <TileLayer
          url="https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png"
          attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors &copy; <a href="https://carto.com/attributions">CARTO</a>'
        />
        {path.length > 0 && <Polyline positions={path} color="#3b82f6" weight={3} />}
        {latestFrame?.gps_fix_valid && latestFrame.gps_lat && latestFrame.gps_lon && (
          <Marker position={[latestFrame.gps_lat, latestFrame.gps_lon]}>
            <Popup className="text-[#0f1117]">
              <div>
                <strong className="block mb-1">Current Position</strong>
                Alt: {latestFrame.gps_alt_m}m<br />
                Speed: {latestFrame.gps_speed_kmh}km/h<br />
                Sats: {latestFrame.gps_satellites}
              </div>
            </Popup>
          </Marker>
        )}
      </MapContainer>

      {/* Stats Overlay */}
      <div className="absolute top-4 right-4 bg-[#1a1d27]/90 border border-[#2a2d3a] p-3 rounded shadow-lg z-[400] text-sm pointer-events-none font-tabular">
        <div className="text-muted mb-1 uppercase text-xs font-bold tracking-wider">Flight Data</div>
        <div className="flex justify-between gap-4"><span>Alt:</span> <span className="text-info">{latestFrame?.gps_alt_m || 0}m</span></div>
        <div className="flex justify-between gap-4"><span>Spd:</span> <span>{latestFrame?.gps_speed_kmh || 0}km/h</span></div>
        <div className="flex justify-between gap-4"><span>Lat:</span> <span>{latestFrame?.gps_lat?.toFixed(5) || '---'}</span></div>
        <div className="flex justify-between gap-4"><span>Lon:</span> <span>{latestFrame?.gps_lon?.toFixed(5) || '---'}</span></div>
      </div>
    </div>
  );
}
