import React from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, ReferenceLine } from 'recharts';
import { useWebSocket } from '../contexts/WebSocketContext';
import { ChartCard } from './ChartCard';

export function AltitudeChart() {
  const { frames, latestFrame } = useWebSocket();
  const data = frames.map(f => ({
    time: new Date(f.received_at).toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', second:'2-digit'}),
    bmp: f.bmp_altitude_m,
    gps: f.gps_alt_m
  }));

  const maxAlt = latestFrame?.max_altitude_m || 0;

  return (
    <ChartCard title="Altitude (m)">
      <ResponsiveContainer width="100%" height="100%">
        <LineChart data={data} margin={{ top: 5, right: 5, left: -20, bottom: 5 }}>
          <CartesianGrid strokeDasharray="3 3" stroke="#2a2d3a" vertical={false} />
          <XAxis dataKey="time" stroke="#71717a" fontSize={12} tick={{fill: '#71717a'}} tickMargin={10} minTickGap={30} />
          <YAxis stroke="#71717a" fontSize={12} tick={{fill: '#71717a'}} />
          <Tooltip 
            contentStyle={{ backgroundColor: '#1a1d27', borderColor: '#2a2d3a', color: '#e4e4e7' }}
            itemStyle={{ color: '#e4e4e7' }}
          />
          {maxAlt > 0 && <ReferenceLine y={maxAlt} stroke="#ef4444" strokeDasharray="3 3" label={{ position: 'top', value: 'Max', fill: '#ef4444', fontSize: 10 }} />}
          <Line type="monotone" dataKey="bmp" name="BMP Alt" stroke="#06b6d4" strokeWidth={2} dot={false} isAnimationActive={false} />
          <Line type="monotone" dataKey="gps" name="GPS Alt" stroke="#3b82f6" strokeWidth={2} dot={false} isAnimationActive={false} />
        </LineChart>
      </ResponsiveContainer>
    </ChartCard>
  );
}
