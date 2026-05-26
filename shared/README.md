# Shared protocol

JSON Schemas describing the wire protocol between the JUCE plugin and the standalone backend over `ws://127.0.0.1:17600`.

All messages share a common envelope (`type` discriminator). See `protocol/envelope.schema.json`.

| Direction | type | Schema |
|---|---|---|
| plugin → server | `register` | `protocol/register.schema.json` |
| plugin → server | `metrics` | `protocol/metrics.schema.json` |
| server → plugin | `command` | `protocol/command.schema.json` |

Other message types from the spec (`referenceSnapshot`, `audioCapture`, `heartbeat`, `recommendation`, `ack`) are deferred — out of scope for the current slice.

These schemas are the single source of truth. Both the C++ plugin (`plugin/Source/Net/Protocol.{h,cpp}`) and the Python backend (`standalone/backend/src/auma_backend/schemas.py`) are written to match them.

Schema version: `0.1`.
