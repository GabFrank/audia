import { useMemo } from "react";

import { useBackendSocket } from "./hooks/useBackendSocket";
import type { InstanceSnapshot } from "./types";

const DEFAULT_WS = "ws://127.0.0.1:17600/ws/frontend";

function formatLufs(value: number | null | undefined): string {
  if (value === null || value === undefined) return "—";
  if (!Number.isFinite(value)) return "—";
  return `${value.toFixed(1)} LUFS`;
}

function InstanceRow({ ins }: { ins: InstanceSnapshot }) {
  const lufs = ins.metrics?.lufsMomentary ?? null;
  const dotColor = ins.connected ? "#5fc466" : "#7a7a7a";
  return (
    <tr style={{ borderBottom: "1px solid #20232b" }}>
      <td style={{ padding: "8px 12px" }}>
        <span
          style={{
            display: "inline-block",
            width: 8,
            height: 8,
            borderRadius: 8,
            background: dotColor,
            marginRight: 8,
          }}
        />
        {ins.trackName ?? ins.instanceId.slice(0, 8)}
      </td>
      <td style={{ padding: "8px 12px", color: "#9aa2b1" }}>{ins.role}</td>
      <td style={{ padding: "8px 12px", color: "#9aa2b1" }}>{ins.format}</td>
      <td
        style={{
          padding: "8px 12px",
          textAlign: "right",
          fontVariantNumeric: "tabular-nums",
          color: lufs === null ? "#7a7a7a" : "#e6e6e6",
        }}
      >
        {formatLufs(lufs as number | null | undefined)}
      </td>
    </tr>
  );
}

export function App() {
  const wsUrl = window.auma?.backendWsUrl ?? DEFAULT_WS;
  const { connection, instances } = useBackendSocket(wsUrl);
  const rows = useMemo(
    () => Object.values(instances).sort((a, b) => a.instanceId.localeCompare(b.instanceId)),
    [instances]
  );

  return (
    <main
      style={{
        fontFamily: "system-ui, -apple-system, sans-serif",
        padding: 24,
        color: "#e6e6e6",
        background: "#101218",
        minHeight: "100vh",
        boxSizing: "border-box",
      }}
    >
      <header style={{ marginBottom: 24, display: "flex", alignItems: "baseline", gap: 16 }}>
        <h1 style={{ margin: 0 }}>AuMA</h1>
        <span style={{ color: "#9aa2b1", fontSize: 13 }}>
          backend: <code>{connection}</code>
        </span>
      </header>

      {rows.length === 0 ? (
        <p style={{ color: "#9aa2b1" }}>
          Waiting for a plugin instance to register. Load the AuMA plugin on any
          track and it should appear here.
        </p>
      ) : (
        <table style={{ borderCollapse: "collapse", width: "100%" }}>
          <thead>
            <tr style={{ textAlign: "left", color: "#7a8295", fontSize: 12 }}>
              <th style={{ padding: "8px 12px" }}>Track</th>
              <th style={{ padding: "8px 12px" }}>Role</th>
              <th style={{ padding: "8px 12px" }}>Format</th>
              <th style={{ padding: "8px 12px", textAlign: "right" }}>LUFS (M)</th>
            </tr>
          </thead>
          <tbody>
            {rows.map((ins) => (
              <InstanceRow key={ins.instanceId} ins={ins} />
            ))}
          </tbody>
        </table>
      )}
    </main>
  );
}
