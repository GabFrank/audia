# auma-app

Electron + React frontend for the AudIA Mastering Assistant standalone.

## Dev loop

Two terminals:

```bash
# 1. Vite dev server (renderer)
cd standalone/electron
npm install
npm run dev

# 2. Electron pointed at the dev server. Picks up either the frozen
#    backend at ../backend/dist/auma-backend/auma-backend or, if it
#    is missing, falls back to spawning python from ../backend/.venv.
npm run dev:electron
```

The backend launcher prefers the frozen binary so the production path
is exercised continuously. To iterate on the backend without freezing,
delete `standalone/backend/dist/` and rely on the venv fallback.

## Production build

```bash
npm run build              # bundles renderer + compiles main process
npm start                  # launches Electron against the built bundle
```
