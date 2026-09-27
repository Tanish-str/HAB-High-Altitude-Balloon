import React, { useEffect } from 'react';
import { StatusHeader } from '../components/StatusHeader';
import { FlightMap } from '../components/FlightMap';
import { AltitudeChart } from '../components/AltitudeChart';
import { TemperatureChart } from '../components/TemperatureChart';
import { PressureChart } from '../components/PressureChart';
import { HumidityChart } from '../components/HumidityChart';
import { BatteryChart } from '../components/BatteryChart';
import { UVChart } from '../components/UVChart';
import { AccelerationChart } from '../components/AccelerationChart';
import { ClimbRateChart } from '../components/ClimbRateChart';
import { DataTable } from '../components/DataTable';
import { FlightTimeline } from '../components/FlightTimeline';
import { FlightSelector } from '../components/FlightSelector';
import { useWebSocket } from '../contexts/WebSocketContext';
import { useFrames } from '../api/hooks';

export function Dashboard() {
  const { flightId, frames, clearFrames } = useWebSocket();
  const { data: historicalFrames } = useFrames(flightId, 2000);

  // If a historical flight is selected and we don't have frames yet,
  // we could theoretically populate them. For this scaffold, we rely on WS for live
  // and the hooks provide historical if needed. We'll stick to WS context for simplicity.

  return (
    <div className="flex flex-col h-screen overflow-hidden bg-bg-primary text-text-primary">
      {/* Sticky Top Header */}
      <div className="sticky top-0 z-50 flex items-center bg-[#1a1d27] border-b border-[#2a2d3a]">
        <div className="flex-1">
          <StatusHeader />
        </div>
        <div className="pr-4 py-2 border-l border-[#2a2d3a]">
          <FlightSelector />
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 overflow-auto p-4">
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-4 h-full min-h-[800px]">
          
          {/* Left Area: Map and Charts */}
          <div className="lg:col-span-2 flex flex-col gap-4 h-full">
            <div className="h-[400px] shrink-0">
              <FlightMap />
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4 flex-1">
              <AltitudeChart />
              <TemperatureChart />
              <PressureChart />
              <HumidityChart />
              <BatteryChart />
              <UVChart />
              <AccelerationChart />
              <ClimbRateChart />
            </div>
          </div>

          {/* Right Area: Timeline and Data Table */}
          <div className="flex flex-col gap-4 h-full">
            <div className="shrink-0">
              <FlightTimeline />
            </div>
            <div className="flex-1 min-h-[400px]">
              <DataTable />
            </div>
          </div>

        </div>
      </div>
    </div>
  );
}
