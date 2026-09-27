const API_BASE = '/api';
const WS_BASE = `ws://${window.location.hostname}:${window.location.port}/ws`;

export async function fetchJson<T>(path: string, options?: RequestInit): Promise<T> {
  const res = await fetch(`${API_BASE}${path}`, {
    ...options,
    headers: { 'Content-Type': 'application/json', ...options?.headers },
  });
  if (!res.ok) throw new Error(`API error: ${res.status} ${res.statusText}`);
  return res.json();
}

export function createWebSocket(onMessage: (data: any) => void): WebSocket {
  const ws = new WebSocket(`${WS_BASE}/live`);
  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      onMessage(data);
    } catch (e) {
      console.error('WebSocket parse error:', e);
    }
  };
  ws.onclose = () => {
    // Auto-reconnect after 2 seconds
    setTimeout(() => createWebSocket(onMessage), 2000);
  };
  ws.onerror = (e) => console.error('WebSocket error:', e);
  return ws;
}
