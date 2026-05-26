"""WebSocket endpoint consumed by the Electron renderer at /ws/frontend.

The protocol is one-way for now (server pushes registry events). It
sends a `snapshot` on connect and then `instance` events on every
change.
"""

from __future__ import annotations

import asyncio
import logging

from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from ..aggregation.registry import InstanceRegistry
from ..schemas import RegistryEvent

logger = logging.getLogger(__name__)


def build_router(registry: InstanceRegistry) -> APIRouter:
    router = APIRouter()

    @router.websocket("/ws/frontend")
    async def frontend_socket(ws: WebSocket) -> None:
        await ws.accept()
        queue = await registry.subscribe()
        try:
            initial = await registry.snapshot()
            await ws.send_json(
                RegistryEvent(type="snapshot", instances=initial).model_dump(
                    exclude_none=True
                )
            )

            while True:
                event = await queue.get()
                await ws.send_json(event.model_dump(exclude_none=True))
        except WebSocketDisconnect:
            pass
        except asyncio.CancelledError:
            raise
        except Exception:
            logger.exception("frontend socket failed")
        finally:
            registry.unsubscribe(queue)

    return router
