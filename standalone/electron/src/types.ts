export type Format = "AU" | "VST3" | "Standalone";
export type Role = "track" | "bus" | "master";

export interface InstanceSnapshot {
  instanceId: string;
  format: Format;
  role: Role;
  trackName?: string;
  sampleRate: number;
  channels: number;
  isFirstInChain?: boolean;
  pluginVersion?: string;
  connected: boolean;
  lastSeen: number;
  metrics: {
    lufsMomentary?: number | null;
    [key: string]: unknown;
  };
}

export type RegistryEvent =
  | { type: "snapshot"; instances: InstanceSnapshot[] }
  | { type: "instance"; instance: InstanceSnapshot }
  | { type: "instanceRemoved"; instanceId: string };

declare global {
  interface Window {
    auma?: {
      backendWsUrl: string;
      backendHttpUrl: string;
    };
  }
}
