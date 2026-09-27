import React, { useMemo } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, ReferenceLine } from 'recharts';
import { useWebSocket } from '../contexts/WebSocketContext';
import { ChartCard } from './ChartCard';

export function ClimbRateChart() {
  const { frames } = useWebSocket();
  
  const data = useMemo(() => {
    const result = [];
    for (let i = 1; i < frames.length; i++) {
      const prev = frames[i - 1];
      const curr = frames[i];
      if (prev.bmp_altitude_m !== null && curr.bmp_altitude_m !== null) {
        const dt = (new Date(curr.received_at).getTime() - new Date(prev.received_at).getTime()) / 1000;
        if (dt > 0) {
          const rate = (curr.bmp_altitude_m - prev.bmp_altitude_m) / dt;
          result.push({
            time: new Date(curr.received_at).toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', second:'2-digit'}),
            rate: Math.max(-50, Math.min(50, rate)) // cap for visual sanity
          });
        }
      }
    }
    return result;
  }, [frames]);

  return (
    <ChartCard title="Climb Rate (m/s)">
      <ResponsiveContainer width="100%" height="100%">
        <LineChart data={data} margin={{ top: 5, right: 5, left: -20, bottom: 5 }}>
          <CartesianGrid strokeDasharray="3 3" stroke="#2a2d3a" vertical={false} />
          <XAxis dataKey="time" stroke="#71717a" fontSize={12} tick={{fill: '#71717a'}} tickMargin={10} minTickGap={30} />
          <YAxis stroke="#71717a" fontSize={12} tick={{fill: '#71717a'}} domain={[-10, 10]} />
          <Tooltip 
            contentStyle={{ backgroundColor: '#1a1d27', borderColor: '#2a2d3a', color: '#e4e4e7' }}
            itemStyle={{ color: '#e4e4e7' }}
          />
          <ReferenceLine y={0} stroke="#71717a" />
          <Line type="monotone" dataKey="rate" name="Rate" stroke="#10b981" strokeWidth={1} dot={false} isAnimationActive={false} />
        </LineChart>
      </ResponsiveContainer>
    </ChartCard>
  );
}
