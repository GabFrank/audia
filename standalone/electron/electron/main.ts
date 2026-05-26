/**
 * Electron main process. Spawns the backend sidecar before opening the
 * window, then loads either the Vite dev server (AUMA_DEV=1) or the
 * built static bundle.
 */

import { BrowserWindow, app } from "electron";
import * as path from "node:path";
import * as process from "node:process";

import { LaunchedBackend, launchBackend, stopBackend } from "./backend-launcher";

const DEV_URL = "http://127.0.0.1:5173";
let backend: LaunchedBackend | null = null;

async function createWindow(): Promise<void> {
  const win = new BrowserWindow({
    width: 1100,
    height: 700,
    backgroundColor: "#101218",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: true,
    },
  });

  if (process.env.AUMA_DEV === "1") {
    await win.loadURL(DEV_URL);
    win.webContents.openDevTools({ mode: "detach" });
  } else {
    await win.loadFile(path.join(__dirname, "..", "dist", "index.html"));
  }
}

app.whenReady().then(async () => {
  try {
    backend = await launchBackend();
    console.log(`[backend] up (${backend.mode}): ${backend.binary}`);
  } catch (err) {
    console.error("[backend] failed to start:", err);
  }
  await createWindow();
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});

app.on("before-quit", () => {
  stopBackend(backend);
  backend = null;
});
