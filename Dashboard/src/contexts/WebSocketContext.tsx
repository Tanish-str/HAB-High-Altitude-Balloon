import React, { createContext, useContext, useEffect, useRef, useState, useCallback } from 'react';
import type { TelemetryFrame } from '../types/telemetry';

interface WebSocketContextType {
  isConnected: boolean;
  latestFrame: TelemetryFrame | null;
  frames: TelemetryFrame[];
  flightId: number | null;
  setFlightId: (id: number | null) => void;
  clearFrames: () => void;
}

const WebSocketContext = createContext<WebSocketContextType | null>(null);

const MAX_FRAMES = 5000;

export function WebSocketProvider({ children }: { children: React.ReactNode }) {
  const [isConnected, setIsConnected] = useState(false);
  const [latestFrame, setLatestFrame] = useState<TelemetryFrame | null>(null);
  const [frames, setFrames] = useState<TelemetryFrame[]>([]);
  const [flightId, setFlightId] = useState<number | null>(null);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeout = useRef<number | null>(null);

  const clearFrames = useCallback(() => {
    setFrames([]);
    setLatestFrame(null);
  }, []);

  useEffect(() => {
    function connect() {
      const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
      const ws = new WebSocket(`${protocol}//${window.location.hostname}:${window.location.port}/ws/live`);

      ws.onopen = () => {
        setIsConnected(true);
        console.log('WebSocket connected');
      };

      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          if (msg.type === 'frame') {
            const frame = msg.data as TelemetryFrame;
            setLatestFrame(frame);
            setFrames(prev => {
              const next = [...prev, frame];
              return next.length > MAX_FRAMES ? next.slice(-MAX_FRAMES) : next;
            });
            if (msg.flight_id && !flightId) {
              setFlightId(msg.flight_id);
            }
          }
        } catch (e) {
          console.error('WS parse error:', e);
        }
      };

      ws.onclose = () => {
        setIsConnected(false);
        console.log('WebSocket disconnected, reconnecting...');
        reconnectTimeout.current = window.setTimeout(connect, 2000);
      };

      ws.onerror = () => {
        ws.close();
      };

      wsRef.current = ws;
    }

    connect();

    return () => {
      if (reconnectTimeout.current) clearTimeout(reconnectTimeout.current);
      wsRef.current?.close();
    };
  }, []);

  return (
    <WebSocketContext.Provider value={{ isConnected, latestFrame, frames, flightId, setFlightId, clearFrames }}>
      {children}
    </WebSocketContext.Provider>
  );
}

export function useWebSocket() {
  const ctx = useContext(WebSocketContext);
  if (!ctx) throw new Error('useWebSocket must be used within WebSocketProvider');
  return ctx;
}
