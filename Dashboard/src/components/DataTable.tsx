import React from 'react';
import { useWebSocket } from '../contexts/WebSocketContext';

export function DataTable() {
  const { frames } = useWebSocket();
  const recent = [...frames].reverse().slice(0, 50);

  return (
    <div className="bg-[#1e2130] border border-[#2a2d3a] rounded-lg overflow-hidden flex flex-col h-full">
      <h3 className="text-sm font-semibold text-muted uppercase tracking-wider p-4 pb-2 border-b border-[#2a2d3a]">Telemetry Log</h3>
      <div className="flex-1 overflow-auto">
        <table className="w-full text-xs font-tabular text-left">
          <thead className="sticky top-0 bg-[#1e2130] text-muted border-b border-[#2a2d3a]">
            <tr>
              <th className="p-2 pl-4">Time</th>
              <th className="p-2">Phase</th>
              <th className="p-2">Alt (m)</th>
              <th className="p-2">Lat</th>
              <th className="p-2">Lon</th>
              <th className="p-2">Spd</th>
              <th className="p-2">Temp</th>
              <th className="p-2">Press</th>
              <th className="p-2 pr-4">Batt</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-[#2a2d3a]">
            {recent.map((f, i) => (
              <tr key={i} className="hover:bg-[#2a2d3a]/50">
                <td className="p-2 pl-4 text-[#a1a1aa] whitespace-nowrap">{new Date(f.received_at).toLocaleTimeString()}</td>
                <td className="p-2">{f.flight_phase.substring(0,3)}</td>
                <td className="p-2 text-info">{f.bmp_altitude_m?.toFixed(1) ?? '-'}</td>
                <td className="p-2">{f.gps_lat?.toFixed(4) ?? '-'}</td>
                <td className="p-2">{f.gps_lon?.toFixed(4) ?? '-'}</td>
                <td className="p-2">{f.gps_speed_kmh?.toFixed(1) ?? '-'}</td>
                <td className="p-2">{f.ds_out_tempC?.toFixed(1) ?? '-'}</td>
                <td className="p-2">{f.bmp_pressPa ? (f.bmp_pressPa/100).toFixed(1) : '-'}</td>
                <td className="p-2 pr-4 text-success">{f.battery_voltage?.toFixed(2) ?? '-'}</td>
              </tr>
            ))}
          </tbody>
        </table>
        {recent.length === 0 && (
          <div className="p-4 text-center text-muted">No data available</div>
        )}
      </div>
    </div>
  );
}
