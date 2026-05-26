/**
 * Preload bridge. Currently only exposes the backend WebSocket URL so
 * the renderer can connect without hard-coding the port.
 */

import { contextBridge } from "electron";

contextBridge.exposeInMainWorld("auma", {
  backendWsUrl: "ws://127.0.0.1:17600/ws/frontend",
  backendHttpUrl: "http://127.0.0.1:17600",
});
