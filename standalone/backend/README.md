# auma-backend

Python sidecar of the AuMA standalone. FastAPI + native WebSockets.

## Endpoints

| Path | Direction | Purpose |
|---|---|---|
| `ws://127.0.0.1:17600/ws/plugin` | plugin → server | telemetry channel (`register`, `metrics`) |
| `ws://127.0.0.1:17600/ws/frontend` | Electron renderer ↔ server | broadcast of registry snapshots |
| `http://127.0.0.1:17600/healthz` | any | liveness probe |
| `http://127.0.0.1:17600/state` | any | one-shot JSON of the current registry |

The server only ever binds to `127.0.0.1`. There is no network exposure.

## Dev install

```bash
cd standalone/backend
python3.11 -m venv .venv
source .venv/bin/activate
pip install -e ".[dev]"
auma-backend                 # starts on 127.0.0.1:17600
```

## Freeze for Electron

```bash
python packaging/build_pyinstaller.py
# produces dist/auma-backend (single-file binary)
```

Electron looks for the binary at `standalone/backend/dist/auma-backend`; if missing, it falls back to invoking the dev script `python -m auma_backend` from a venv.
