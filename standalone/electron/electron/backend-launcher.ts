/**
 * Locates and launches the AuMA backend sidecar.
 *
 * Strategy (in order):
 *   1. Look for the PyInstaller --onedir bundle at
 *      standalone/backend/dist/auma-backend/auma-backend (relative to
 *      the repository root when running in dev, or inside resources
 *      when packaged).
 *   2. Fall back to invoking `python -m auma_backend` from a developer
 *      venv at standalone/backend/.venv so the backend can be iterated
 *      without re-freezing.
 *
 * The launcher waits until GET /healthz returns 200, or rejects after
 * a timeout. Either way the spawned process is tracked and killed on
 * Electron's `before-quit`.
 */

import { ChildProcess, spawn } from "node:child_process";
import { existsSync } from "node:fs";
import * as http from "node:http";
import * as path from "node:path";
import * as process from "node:process";

const BACKEND_HOST = "127.0.0.1";
const BACKEND_PORT = 17600;
const READY_TIMEOUT_MS = 30_000;

export interface LaunchedBackend {
  process: ChildProcess;
  mode: "frozen" | "venv";
  binary: string;
}

function repoRoot(): string {
  // Electron main is built to standalone/electron/dist-electron/main.js,
  // so two parents above lands at the electron/ root; three at standalone/;
  // four at the repository root.
  return path.resolve(__dirname, "..", "..", "..");
}

function frozenBinaryPath(): string {
  const fromResources = path.join(
    process.resourcesPath ?? "",
    "auma-backend",
    "auma-backend"
  );
  if (process.resourcesPath && existsSync(fromResources)) {
    return fromResources;
  }
  return path.join(
    repoRoot(),
    "standalone",
    "backend",
    "dist",
    "auma-backend",
    "auma-backend"
  );
}

function venvPython(): string {
  return path.join(
    repoRoot(),
    "standalone",
    "backend",
    ".venv",
    "bin",
    "python"
  );
}

function backendSrcPath(): string {
  return path.join(repoRoot(), "standalone", "backend", "src");
}

async function waitForReady(timeoutMs: number): Promise<void> {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    const ok = await new Promise<boolean>((resolve) => {
      const req = http.get(
        { host: BACKEND_HOST, port: BACKEND_PORT, path: "/healthz", timeout: 1000 },
        (res) => {
          res.resume();
          resolve(res.statusCode === 200);
        }
      );
      req.on("error", () => resolve(false));
      req.on("timeout", () => {
        req.destroy();
        resolve(false);
      });
    });
    if (ok) return;
    await new Promise((r) => setTimeout(r, 300));
  }
  throw new Error(`backend not ready after ${timeoutMs} ms`);
}

export async function launchBackend(): Promise<LaunchedBackend> {
  const frozen = frozenBinaryPath();
  const env = {
    ...process.env,
    AUMA_HOST: BACKEND_HOST,
    AUMA_PORT: String(BACKEND_PORT),
    PYTHONUNBUFFERED: "1",
  };

  let child: ChildProcess;
  let mode: "frozen" | "venv";
  let binary: string;

  if (existsSync(frozen)) {
    mode = "frozen";
    binary = frozen;
    child = spawn(frozen, [], { env, stdio: "inherit" });
  } else {
    const py = venvPython();
    if (!existsSync(py)) {
      throw new Error(
        `no backend found. Tried frozen at ${frozen} and venv at ${py}.`
      );
    }
    mode = "venv";
    binary = py;
    child = spawn(py, ["-m", "auma_backend"], {
      env: { ...env, PYTHONPATH: backendSrcPath() },
      stdio: "inherit",
    });
  }

  child.on("exit", (code, signal) => {
    console.warn(`[backend] exited code=${code} signal=${signal}`);
  });

  try {
    await waitForReady(READY_TIMEOUT_MS);
  } catch (err) {
    if (!child.killed) child.kill();
    throw err;
  }

  return { process: child, mode, binary };
}

export function stopBackend(handle: LaunchedBackend | null): void {
  if (!handle) return;
  if (handle.process.killed) return;
  handle.process.kill();
}
