import React, { useMemo } from 'react';
import { useWebSocket } from '../contexts/WebSocketContext';
import { FlightPhase } from '../types/telemetry';

const PHASES: FlightPhase[] = ['PRELAUNCH', 'ASCENT', 'NEAR_APOGEE', 'DESCENT', 'LANDED'];

export function FlightTimeline() {
  const { frames, latestFrame } = useWebSocket();

  const phaseTimes = useMemo(() => {
    const times: Partial<Record<FlightPhase, string>> = {};
    for (const f of frames) {
      if (!times[f.flight_phase]) {
        times[f.flight_phase] = new Date(f.received_at).toLocaleTimeString();
      }
    }
    return times;
  }, [frames]);

  const currentPhase = latestFrame?.flight_phase || 'PRELAUNCH';
  const currentIndex = PHASES.indexOf(currentPhase);

  return (
    <div className="bg-[#1e2130] border border-[#2a2d3a] rounded-lg p-4">
      <h3 className="text-sm font-semibold text-muted uppercase tracking-wider mb-4">Mission Timeline</h3>
      <div className="relative flex justify-between items-start w-full mt-2">
        <div className="absolute top-2 left-0 w-full h-[2px] bg-[#2a2d3a] -z-10"></div>
        {PHASES.map((phase, i) => {
          const isReached = i <= currentIndex;
          const isCurrent = i === currentIndex;
          const time = phaseTimes[phase];
          
          return (
            <div key={phase} className="flex flex-col items-center flex-1">
              <div className={`w-4 h-4 rounded-full mb-2 ${isCurrent ? 'bg-accent shadow-[0_0_10px_rgba(59,130,246,0.5)]' : isReached ? 'bg-success' : 'bg-[#2a2d3a]'}`} />
              <div className={`text-[10px] font-bold text-center uppercase ${isReached ? 'text-primary' : 'text-muted'}`}>
                {phase.replace('_', ' ')}
              </div>
              <div className="text-[10px] text-muted font-tabular mt-1 h-3">
                {time || ''}
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}
