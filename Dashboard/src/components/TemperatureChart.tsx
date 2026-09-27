import React from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { useWebSocket } from '../contexts/WebSocketContext';
import { ChartCard } from './ChartCard';

export function TemperatureChart() {
  const { frames } = useWebSocket();
  const data = frames.map(f => ({
    time: new Date(f.received_at).toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', second:'2-digit'}),
    aht: f.aht_tempC,
    ds_in: f.ds_in_tempC,
    ds_out: f.ds_out_tempC,
    dew: f.dew_point_C
  }));

  return (
    <ChartCard title="Temperature (°C)">
      <ResponsiveContainer width="100%" height="100%">
        <LineChart data={data} margin={{ top: 5, right: 5, left: -20, bottom: 5 }}>
          <CartesianGrid strokeDasharray="3 3" stroke="#2a2d3a" vertical={false} />
          <XAxis dataKey="time" stroke="#71717a" fontSize={12} tick={{fill: '#71717a'}} tickMargin={10} minTickGap={30} />
          <YAxis stroke="#71717a" fontSize={12} tick={{fill: '#71717a'}} />
          <Tooltip 
            contentStyle={{ backgroundColor: '#1a1d27', borderColor: '#2a2d3a', color: '#e4e4e7' }}
            itemStyle={{ color: '#e4e4e7' }}
          />
          <Line type="monotone" dataKey="aht" name="AHT10" stroke="#f59e0b" strokeWidth={2} dot={false} isAnimationActive={false} />
          <Line type="monotone" dataKey="ds_in" name="Inside" stroke="#22c55e" strokeWidth={2} dot={false} isAnimationActive={false} />
          <Line type="monotone" dataKey="ds_out" name="Outside" stroke="#3b82f6" strokeWidth={2} dot={false} isAnimationActive={false} />
          <Line type="monotone" dataKey="dew" name="Dew Pt" stroke="#06b6d4" strokeWidth={1} strokeDasharray="3 3" dot={false} isAnimationActive={false} />
        </LineChart>
      </ResponsiveContainer>
    </ChartCard>
  );
}
