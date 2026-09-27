import { useQuery } from '@tanstack/react-query';
import { fetchJson } from './client';
import type { Flight, TelemetryFrame, HealthStatus } from '../types/telemetry';

export function useFlights() {
  return useQuery<Flight[]>({
    queryKey: ['flights'],
    queryFn: () => fetchJson('/flights'),
    refetchInterval: 10000,
  });
}

export function useFlight(id: number | null) {
  return useQuery<Flight>({
    queryKey: ['flight', id],
    queryFn: () => fetchJson(`/flights/${id}`),
    enabled: id !== null,
    refetchInterval: 5000,
  });
}

export function useFrames(flightId: number | null, limit = 1000) {
  return useQuery<TelemetryFrame[]>({
    queryKey: ['frames', flightId, limit],
    queryFn: () => fetchJson(`/flights/${flightId}/frames?limit=${limit}&order=asc`),
    enabled: flightId !== null,
    refetchInterval: false, // We use WebSocket for live updates
  });
}

export function useLatestFrame(flightId: number | null) {
  return useQuery<TelemetryFrame>({
    queryKey: ['latestFrame', flightId],
    queryFn: () => fetchJson(`/flights/${flightId}/frames/latest`),
    enabled: flightId !== null,
    refetchInterval: 3000,
  });
}

export function useHealth() {
  return useQuery<HealthStatus>({
    queryKey: ['health'],
    queryFn: () => fetchJson('/health'),
    refetchInterval: 5000,
  });
}
