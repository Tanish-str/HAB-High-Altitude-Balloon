import React from 'react';
import { useWebSocket } from '../contexts/WebSocketContext';
import { parseSensorHealth } from '../types/telemetry';

export function StatusHeader() {
  const { isConnected, latestFrame } = useWebSocket();

  const phaseColors: Record<string, string> = {
    PRELAUNCH: 'bg-slate-600',
    ASCENT: 'bg-green-500',
    NEAR_APOGEE: 'bg-amber-500',
    DESCENT: 'bg-blue-500',
    LANDED: 'bg-red-500'
  };

  const health = latestFrame ? parseSensorHealth(latestFrame.sensor_health_hex) : {};
  const isStale = latestFrame ? (new Date().getTime() - new Date(latestFrame.received_at).getTime() > 6000) : false;

  return (
    <header className="sticky top-0 z-50 bg-[#1a1d27] border-b border-[#2a2d3a] p-3 flex flex-wrap items-center gap-4 text-sm font-tabular">
      <div className="flex items-center gap-2">
        <div className={`w-3 h-3 rounded-full ${isConnected ? 'bg-success' : 'bg-danger'}`} />
        <span>WS</span>
      </div>

      {latestFrame ? (
        <>
          <div className={`px-2 py-1 rounded text-white font-bold ${phaseColors[latestFrame.flight_phase] || 'bg-slate-600'}`}>
            {latestFrame.flight_phase}
          </div>

          <div className={`flex items-center gap-2 px-3 border-l border-r border-[#2a2d3a] ${isStale ? 'text-warning font-bold' : ''}`}>
            Last Frame: {new Date(latestFrame.received_at).toLocaleTimeString()}
            {isStale && ' (STALE)'}
          </div>

          <div className="flex items-center gap-1">
            {Object.entries(health).map(([name, isOk]) => (
              <div key={name} title={name} className={`w-3 h-3 rounded-sm ${isOk ? 'bg-success' : 'bg-danger'}`} />
            ))}
          </div>

          <div className="flex gap-4 ml-auto border-l border-[#2a2d3a] pl-4">
            <div>
              <span className="text-muted mr-1">ALT</span>
              <span className="text-info font-bold">{latestFrame.bmp_altitude_m?.toFixed(0) || '---'} m</span>
            </div>
            <div>
              <span className="text-muted mr-1">SPD</span>
              <span className="font-bold">{latestFrame.gps_speed_kmh?.toFixed(0) || '---'} km/h</span>
            </div>
            <div>
              <span className="text-muted mr-1">SAT</span>
              <span className="font-bold">{latestFrame.gps_satellites}</span>
            </div>
            <div className="text-xs flex flex-col justify-center">
              <div className="text-success leading-tight">OK: {latestFrame.rx_frames_ok}</div>
              <div className="text-danger leading-tight">ERR: {latestFrame.rx_frames_bad_checksum}</div>
            </div>
          </div>
        </>
      ) : (
        <div className="ml-4 text-muted">Waiting for data...</div>
      )}
    </header>
  );
}
