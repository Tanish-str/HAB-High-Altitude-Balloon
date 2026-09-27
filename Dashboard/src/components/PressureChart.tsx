import React from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { useWebSocket } from '../contexts/WebSocketContext';
import { ChartCard } from './ChartCard';

export function PressureChart() {
  const { frames } = useWebSocket();
  const data = frames.map(f => ({
    time: new Date(f.received_at).toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', second:'2-digit'}),
    pressure: f.bmp_pressPa ? f.bmp_pressPa / 100 : null // hPa
  }));

  return (
    <ChartCard title="Pressure (hPa)">
      <ResponsiveContainer width="100%" height="100%">
        <LineChart data={data} margin={{ top: 5, right: 5, left: -20, bottom: 5 }}>
          <CartesianGrid strokeDasharray="3 3" stroke="#2a2d3a" vertical={false} />
          <XAxis dataKey="time" stroke="#71717a" fontSize={12} tick={{fill: '#71717a'}} tickMargin={10} minTickGap={30} />
          <YAxis stroke="#71717a" fontSize={12} tick={{fill: '#71717a'}} domain={['auto', 'auto']} />
          <Tooltip 
            contentStyle={{ backgroundColor: '#1a1d27', borderColor: '#2a2d3a', color: '#e4e4e7' }}
            itemStyle={{ color: '#e4e4e7' }}
          />
          <Line type="monotone" dataKey="pressure" name="Pressure" stroke="#a855f7" strokeWidth={2} dot={false} isAnimationActive={false} />
        </LineChart>
      </ResponsiveContainer>
    </ChartCard>
  );
}
