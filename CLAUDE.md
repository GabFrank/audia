# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Estado actual

Repo en fase de diseño. **Solo existe** `docs/audire-documento-implementacion.md` — spec completo de implementación. No hay código, build, ni tests todavía. Cualquier estructura/comando concreto debe nacer alineado a la spec; leerla antes de andamiar nada.

## Qué es Audire

Sistema open-source (GPLv3, sin fines de lucro) de asistencia a mezcla/mastering. Dos piezas:

- **Plugin** (JUCE C++, AU + VST3): tap transparente del audio en cada pista + master, mide métricas baratas en vivo y captura audio on-demand.
- **Standalone** (Electron + React + sidecar Python/FastAPI): agrega telemetría de todas las instancias, corre análisis profundo (DSP/MIR/psicoacústica), genera espectrogramas, razona con Claude API (BYO-key), y devuelve recomendaciones por pista y de master sin tocar la sesión del DAW.

"Plugin mide; standalone piensa."

## Arquitectura — invariantes no negociables

Antes de tocar código que ya exista, validar contra estos puntos (sec. referenciadas en la spec):

- **Audio thread del plugin (§3.2):** `processBlock` prohibido allocar memoria, tomar locks, hacer I/O o red. Solo: copia a `juce::AbstractFifo` lock-free, escritura a ring buffer si hay grabación, y contadores baratos. Todo lo demás (FFT pesada, serialización, envío WebSocket) corre en worker thread.
- **Transporte plugin↔standalone (§5):** WebSocket sobre `ws://127.0.0.1:<puerto>` (default 17600). Plugins = clientes, standalone = servidor. JSON para control/métricas, frames binarios fragmentados para audio. Loopback **solo** — nunca exponer a red.
- **Streaming de audio:** descartado continuo. Solo captura on-demand (mono 48 kHz para MIR/psicoacústica, L/R solo para análisis estéreo/fase).
- **Identidad de instancia (§3.5, §3.8):** `instanceId` es UUID estable persistido vía `getStateInformation`/`setStateInformation` — sobrevive guardar/reabrir el proyecto del DAW. No regenerar al reconectar.
- **Sidecar Python (§1.1, §6.3):** Python es no negociable por dependencias (`pyloudnorm`, `librosa`, `Essentia`, `MoSQITo`); reimplementar en Node es inviable. Empaquetado: PyInstaller embebido en el recurso Electron, arrancado por `child_process`.
- **BYO-key (§4.7, §10):** API keys (Claude, Gemini) guardadas en Keychain/almacén seguro del SO vía API segura de Electron. Nunca texto plano, nunca al repo.
- **Llamadas al agente (§4.7):** on-demand o al fijar versión — **nunca por buffer**. Salida estructurada en JSON, parseada de forma segura. Sugerencias rechazadas se inyectan al prompt para no repetirse.
- **Privacidad (§10):** Claude recibe métricas + espectrogramas, no audio crudo. Gemini sí recibe audio → opt-in explícito. Qwen2-Audio = alternativa 100% local.
- **Licencia (§14):** GPLv3 obligatorio (JUCE-GPL + Essentia-AGPL). Modelos MTG son CC BY-NC-ND 4.0 → solo inferencia, con atribución, sin redistribuir modificados, sin uso comercial.

## Layout del monorepo (planeado, §12)

```
/plugin       JUCE C++ (AU + VST3) — CMake
/standalone
  /electron   shell + frontend React/Next
  /backend    Python FastAPI (server, analysis, aggregation, agent, describer, storage)
/shared       esquemas JSON del protocolo plugin↔standalone
/docs
```

Cuando armes algo nuevo, respetar esta división. El esquema del protocolo WebSocket vive en `/shared` para que plugin y backend lo compartan.

## Modelo de datos (§4.10)

SQLite local. Entidades clave: `sessions`, `tracks`, `versions` (con `parent_version_id` para ramas, `is_reference` para v0 crudo), `metrics`, `recommendations` (status: `pending|applied|rejected` — `rejected` alimenta memoria del agente), `descriptions`, `references`, `project_snapshots`, `snapshot_versions`, `spectrograms`.

## Roadmap (§11) — para priorizar

- **MVP:** plugin con métricas vivas + captura; standalone con telemetría/agregación/análisis de loudness/dinámica/espectral/estéreo/defectos/compliance; agente Claude por pista; versionamiento lineal; A/B; modo Mix.
- **Fase 1 (diferenciador):** psicoacústica (MoSQITo), masking cruzado, modo Master.
- **Fase 2:** MIR tonal/rítmico, comparativos avanzados, árbol con ramas, reporte PDF.
- **Opcional:** describer Gemini/Qwen.
