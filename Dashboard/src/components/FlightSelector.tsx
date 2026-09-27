import React from 'react';
import { useFlights } from '../api/hooks';
import { useWebSocket } from '../contexts/WebSocketContext';

export function FlightSelector() {
  const { data: flights } = useFlights();
  const { flightId, setFlightId, clearFrames } = useWebSocket();

  return (
    <div className="flex items-center gap-2 text-sm ml-4 border-l border-[#2a2d3a] pl-4">
      <span className="text-muted">Flight:</span>
      <select 
        className="bg-[#1e2130] border border-[#2a2d3a] rounded px-2 py-1 text-primary focus:outline-none focus:border-accent"
        value={flightId || ''}
        onChange={(e) => {
          const val = e.target.value;
          clearFrames();
          if (val === '') {
            setFlightId(null);
          } else {
            setFlightId(parseInt(val, 10));
          }
        }}
      >
        <option value="">-- Live / Auto --</option>
        {flights?.map(f => (
          <option key={f.id} value={f.id}>
            #{f.id} {f.name} ({new Date(f.started_at).toLocaleDateString()})
          </option>
        ))}
      </select>
    </div>
  );
}
