# AudIA Mastering Assistant (AuMA)

Open-source mix and mastering assistant. Audio plugin (AU + VST3) plus a standalone app that aggregates metrics, reasons with an LLM, and recommends signal-chain changes — without touching the DAW session.

> Codename in the original spec: **Audire**. Current project name: **AudIA Mastering Assistant**, short **AuMA**. The spec document under `docs/` is the source of truth and still uses the original codename.

## Status

Early stage. This repository currently delivers a vertical slice end-to-end: a JUCE plugin that streams momentary LUFS over WebSocket to a Python backend, surfaced live in an Electron + React UI. Deep analysis, the Claude agent, the semantic describer, and versioning are deliberately out of scope until the transport and packaging paths are validated.

See `docs/audire-documento-implementacion.md` for the full design.

## Repository layout

```
plugin/         JUCE C++ plugin (AU + VST3)
standalone/
  backend/     Python FastAPI sidecar
  electron/    Electron shell + React frontend
shared/         JSON Schemas for the plugin ↔ standalone protocol
scripts/        Developer convenience scripts
docs/           Design documents
```

## License

GPL-3.0-only. See `LICENSE`.

The GPL choice is forced by upstream dependencies (JUCE-GPL, Essentia-AGPL) and accepted intentionally. This is a non-profit, open project.
