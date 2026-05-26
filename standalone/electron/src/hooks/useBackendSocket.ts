import { useEffect, useRef, useState } from "react";

import type { InstanceSnapshot, RegistryEvent } from "../types";

type ConnectionState = "connecting" | "open" | "closed";

const RECONNECT_DELAY_MS = 1500;

export interface BackendSocketState {
  connection: ConnectionState;
  instances: Record<string, InstanceSnapshot>;
}

export function useBackendSocket(url: string): BackendSocketState {
  const [state, setState] = useState<BackendSocketState>({
    connection: "connecting",
    instances: {},
  });
  const retryTimer = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(() => {
    let cancelled = false;
    let ws: WebSocket | null = null;

    const connect = () => {
      if (cancelled) return;
      setState((s) => ({ ...s, connection: "connecting" }));
      ws = new WebSocket(url);

      ws.onopen = () => {
        if (cancelled) return;
        setState((s) => ({ ...s, connection: "open" }));
      };

      ws.onmessage = (ev) => {
        if (cancelled) return;
        let event: RegistryEvent;
        try {
          event = JSON.parse(ev.data) as RegistryEvent;
        } catch {
          return;
        }
        setState((prev) => {
          if (event.type === "snapshot") {
            const next: Record<string, InstanceSnapshot> = {};
            for (const ins of event.instances) next[ins.instanceId] = ins;
            return { ...prev, instances: next };
          }
          if (event.type === "instance") {
            return {
              ...prev,
              instances: {
                ...prev.instances,
                [event.instance.instanceId]: event.instance,
              },
            };
          }
          if (event.type === "instanceRemoved") {
            const next = { ...prev.instances };
            delete next[event.instanceId];
            return { ...prev, instances: next };
          }
          return prev;
        });
      };

      ws.onclose = () => {
        if (cancelled) return;
        setState((s) => ({ ...s, connection: "closed" }));
        retryTimer.current = setTimeout(connect, RECONNECT_DELAY_MS);
      };

      ws.onerror = () => {
        // Let onclose drive the reconnect — closing twice is harmless.
        ws?.close();
      };
    };

    connect();
    return () => {
      cancelled = true;
      if (retryTimer.current) clearTimeout(retryTimer.current);
      ws?.close();
    };
  }, [url]);

  return state;
}
