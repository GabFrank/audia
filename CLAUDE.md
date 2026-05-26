# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

**AudIA Mastering Assistant (AuMA).** Open-source mix and mastering assistant. JUCE audio plugin (AU + VST3) that taps each track and master, plus an Electron + React + Python standalone that aggregates telemetry, runs deep analysis, reasons with an LLM, and recommends signal-chain changes — without touching the DAW session.

The original design document is `docs/audire-documento-implementacion.md` (still uses the codename "Audire") and remains the source of truth for architecture decisions. **Read it before scaffolding anything new.**

## Current state — vertical slice complete

This repository delivers an end-to-end slice, deliberately narrow:

- A JUCE plugin (AU + VST3) measures **momentary LUFS** in real time and streams `register` + `metrics` frames over WebSocket to the standalone.
- A FastAPI backend on `ws://127.0.0.1:17600` registers instances and re-broadcasts changes to the Electron renderer.
- An Electron + React app shows a live per-instance LUFS table.
- The backend ships as a PyInstaller `--onedir` bundle; the Electron launcher prefers the frozen binary in production and falls back to a developer venv otherwise.

**Out of scope until further notice:** deep analysis (Essentia, MoSQITo, MIR), Claude agent, semantic describer, SQLite persistence and versioning. Don't add them without a discussion.

## Layout

```
plugin/                   JUCE 7.0.5, C++17, CMake. AU + VST3.
  Source/
    PluginProcessor      realtime tap, FIFO push, worker timer
    PluginEditor         minimal UI (current LUFS readout)
    Metrics/             K-weighting + LufsMeter (worker-side)
    Net/                 Protocol JSON + WebSocketClient (ixwebsocket)

standalone/
  backend/                Python 3.11+, FastAPI, no [standard] extras.
    src/auma_backend/
      app.py             FastAPI factory, /healthz, /state
      schemas.py         pydantic mirrors of /shared/protocol
      server/            /ws/plugin and /ws/frontend
      aggregation/       in-memory InstanceRegistry with pub/sub
    packaging/
      build_pyinstaller.py   freeze script (onedir)
      entrypoint.py          PyInstaller shim outside the package

  electron/               Vite + React + TS renderer, Electron main.
    electron/             main.ts, preload.ts, backend-launcher.ts
    src/                  App + useBackendSocket hook + types

shared/protocol/          JSON Schemas — single source of truth for
                          register / metrics / command frames
                          (consumed by both plugin and backend).
docs/                     Design doc (Spanish).
```

## Invariants — do not violate

These come from `docs/audire-documento-implementacion.md` and from the traps already paid for during the slice. Validate any change against them.

- **Audio thread (§3.2).** `AumaProcessor::processBlock` must never allocate, lock, do IO or network. The realtime path is a single `juce::AbstractFifo` push into `LufsMeter`. All heavy lifting (K-weighting, window slide, WebSocket send) is on the worker `juce::Timer` running on the message thread.
- **Loopback only (§10, §5).** The backend binds `127.0.0.1` exclusively. The plugin connects to `ws://127.0.0.1:17600/ws/plugin`. Don't expose any port to the network.
- **`instanceId` is persistent (§3.5, §3.8).** It's a UUID generated on first construction and round-tripped through `getStateInformation`/`setStateInformation`. Survives DAW project save/reload. Never regenerate it on reconnect.
- **Streaming audio is on-demand only (§3.4).** The slice carries no audio frames yet; when capture lands, it stays mono 48 kHz for MIR/psychoacoustic features and is triggered only by an explicit user action.
- **BYO API keys (§4.7, §10).** Claude / Gemini keys live in the OS keychain via Electron's safeStorage. Never plaintext, never in the repo.
- **GPL-3.0-only (§14).** JUCE-GPL and Essentia-AGPL force this. Keep `LICENSE` and dependency lineage clean.

## Conventions

- English everywhere — code, identifiers, comments, commit messages, public docs. (User-facing UI copy may be Spanish later; we'll mark that explicitly.)
- **Conventional Commits.** `feat(scope)`, `fix(scope)`, `docs(scope)`, `chore`, `build`, `refactor`. The scope tracks the layer: `plugin`, `backend`, `electron`, `shared`.
- macOS-first (Logic Pro). Other platforms come along when free.
- Atomic commits per layer. The vertical slice was ten commits; keep that cadence.

## Dev loop

### Backend
```bash
cd standalone/backend
python3.11 -m venv .venv
source .venv/bin/activate
pip install -e ".[dev]"
auma-backend                        # serves 127.0.0.1:17600
```
Freeze for Electron production:
```bash
python packaging/build_pyinstaller.py    # → dist/auma-backend/auma-backend
```

### Electron + renderer
```bash
cd standalone/electron
npm install
npm run dev                         # Vite renderer at 127.0.0.1:5173
npm run dev:electron                # Electron, launches backend automatically
```
The Electron launcher tries the frozen binary first, then falls back to spawning `python -m auma_backend` from `standalone/backend/.venv`.

### Plugin
```bash
cd plugin
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target auma_plugin_AU auma_plugin_VST3 -j
```
With `COPY_PLUGIN_AFTER_BUILD`, the artifacts land at:
- `~/Library/Audio/Plug-Ins/Components/AuMA.component` (AU)
- `~/Library/Audio/Plug-Ins/VST3/AuMA.vst3` (VST3)

Restart the DAW after the first AU install.

## Trap notes (paid for during the slice)

- **PyInstaller + uvicorn[standard]:** uvloop is a Cython extension that **hangs at import** inside a frozen bundle on macOS. The backend uses plain `uvicorn` and pins `loop="asyncio", http="h11", ws="websockets"`. The freeze script also `--exclude-module`s uvloop / watchfiles / httptools.
- **PyInstaller --onefile on macOS:** each `dlopen` triggers an Endpoint Security notification that stalls for seconds when Apple's verification endpoints are unreachable. We ship `--onedir`. Don't switch back without testing offline.
- **JUCE / Xcode 15.0:** JUCE 7.0.9+ and JUCE 8 refuse to compile under Xcode 15.0 (broken linker). JUCE is pinned to 7.0.5 in `plugin/CMakeLists.txt`. Bump when the toolchain moves.
- **C++17 only:** the plugin's compile standard is C++17 because of JUCE 7.0.5. `std::numbers` is C++20 — use plain constants.

## Protocol contract

The frames on `/ws/plugin` are defined by the JSON Schemas in `shared/protocol/`. Both `plugin/Source/Net/Protocol.{h,cpp}` and `standalone/backend/src/auma_backend/schemas.py` are hand-written to match — keep them aligned when you touch any of the three.

Reserved message types from the spec (`referenceSnapshot`, `audioCapture`, `heartbeat`, `recommendation`, `ack`) are not implemented yet. Add the schema first, then the producer, then the consumer.
