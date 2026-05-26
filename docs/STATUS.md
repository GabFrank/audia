# Project status

Updated end of session 1 — vertical slice complete.

## TL;DR

End-to-end transport + packaging path validated. Plugin → WebSocket → backend → Electron UI carries **momentary LUFS only**. Ten atomic commits pushed to `main`. Deep analysis, Claude agent, semantic describer and SQLite versioning are deliberately not started.

## What works today

| Component | State | Where |
|---|---|---|
| Protocol JSON schemas (register / metrics / command) | ✅ defined, mirrored in C++ and Python | `shared/protocol/` |
| Python backend (FastAPI + WebSocket) | ✅ runs on `127.0.0.1:17600`; `/healthz`, `/state`, `/ws/plugin`, `/ws/frontend` | `standalone/backend/` |
| PyInstaller `--onedir` freeze of backend | ✅ produces `dist/auma-backend/auma-backend`, boots in ~3 s | `standalone/backend/packaging/` |
| Electron shell with backend launcher (frozen + dev venv fallback) | ✅ TypeScript build green | `standalone/electron/electron/` |
| React UI showing live per-instance LUFS table | ✅ Vite build green; reconnect on backend restart | `standalone/electron/src/` |
| JUCE plugin (AU + VST3), realtime tap, lock-free FIFO | ✅ AU + VST3 build & install to `~/Library/Audio/Plug-Ins/{Components,VST3}/` | `plugin/` |
| K-weighting + momentary LUFS meter (BS.1770 / EBU R128, 400 ms window) | ✅ on worker thread, atomic publish | `plugin/Source/Metrics/` |
| WebSocket client in plugin (ixwebsocket), register + metrics frames | ✅ end-to-end smoke test passed | `plugin/Source/Net/` |
| Persistent `instanceId` across DAW save/reload | ✅ `juce::ValueTree` in `getState/setState` | `plugin/Source/PluginProcessor.cpp` |
| CLAUDE.md with invariants, dev loop, traps | ✅ rewritten in commit 10 | `CLAUDE.md` |

## What is deferred (per slice scope)

- Deep analysis: Essentia, `pyloudnorm`, MoSQITo, MIR, integrated LUFS, LRA, true peak, spectrograms.
- Cross-track aggregation: spectral masking, gain-staging, project-wide balance.
- Claude agent layer: prompt orchestration, structured JSON recommendations, history of rejected suggestions.
- Semantic describer: Gemini API client, Qwen2-Audio local fallback, regenerate / refine UI.
- Versioning: SQLite schema (`sessions`, `tracks`, `versions`, `recommendations`, ...), the three-level versioning tree, snapshots.
- Compare / Master / Report dashboard modes.
- BYO API-key UI bound to the OS keychain via Electron `safeStorage`.
- macOS plugin notarization, code-signing for distribution.
- Audio capture on demand (full-track passes from plugin to backend).
- Heartbeat, `referenceSnapshot`, `audioCapture`, `recommendation`, `ack` protocol messages — schemas are reserved but not implemented.

## Manual verification still pending

The slice was smoke-tested with a Python client that mimics the plugin frames. To close the loop with the actual binary, do this once and note the result here:

1. `cd standalone/electron && npm install && npm run build:web && npm run dev:electron` (this launches the backend automatically).
2. Open Logic Pro, insert **AuMA** on any track, hit play.
3. Confirm the Electron window lists the instance and the LUFS value updates ~3 times per second.
4. Compare against Logic's built-in loudness meter on the same source. The reading should agree within ≈ 0.5 LUFS.

If anything is off, capture the symptom in this file before next session.

## Known traps already paid for

Each is also encoded in the relevant CMakeLists / build script / commit body. Listed here for fast recall.

- **uvloop hangs at import inside a PyInstaller bundle on macOS.** Backend dropped `uvicorn[standard]` and pins `loop="asyncio", http="h11", ws="websockets"`. Don't reinstate `[standard]`.
- **PyInstaller `--onefile` stalls on every `dlopen`** when Apple's verification endpoints aren't reachable. We ship `--onedir`. Don't switch back without testing offline.
- **Xcode 15.0 toolchain is incompatible with JUCE 7.0.9+ and JUCE 8.** Pinned at JUCE 7.0.5 in `plugin/CMakeLists.txt`. Bump when Xcode moves.
- **JUCE 7.0.5 enforces C++17.** No `std::numbers`, no C++20-only constructs in plugin code.

## Suggested next steps (any order)

1. **Manual LUFS validation in Logic** (above). Until that's done, the slice is unverified against a real DAW.
2. **Add True Peak + Integrated LUFS** as a second metric, on both plugin and backend. Stays in the existing protocol — `metrics.metrics` is open. Cheapest path to a more useful UI.
3. **Persist `instanceId` per-track display name** entered in the plugin's local UI so the dashboard doesn't show UUID prefixes when the DAW can't provide a name.
4. **SQLite scaffolding (no versioning yet)** so the backend keeps a flat history of metric frames across restarts. Useful for the next bigger step (versioning).
5. **Start the Python deep-analysis layer** behind a single `analyze_full(track_pcm) → metrics_blob` endpoint, with `pyloudnorm` as the first real implementation. No agent yet.

Discuss before picking which of these to do next — some imply protocol additions and others don't.

## Repository pointers for the next session

- `docs/audire-documento-implementacion.md` — original Spanish design doc, source of truth.
- `CLAUDE.md` — invariants, conventions, traps, dev loop. Read this before scaffolding anything.
- `shared/protocol/` — single source of truth for wire format.
- This file (`docs/STATUS.md`) — keep updating it at the end of each session.
