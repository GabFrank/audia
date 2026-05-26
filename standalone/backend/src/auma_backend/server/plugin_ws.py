"""WebSocket endpoint exposed to plugin instances at /ws/plugin."""

from __future__ import annotations

import logging
from typing import Optional

from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from pydantic import TypeAdapter, ValidationError

from ..aggregation.registry import InstanceRegistry
from ..schemas import Metrics, PluginMessage, Register

logger = logging.getLogger(__name__)
_adapter: TypeAdapter[PluginMessage] = TypeAdapter(PluginMessage)


def build_router(registry: InstanceRegistry) -> APIRouter:
    router = APIRouter()

    @router.websocket("/ws/plugin")
    async def plugin_socket(ws: WebSocket) -> None:
        await ws.accept()
        instance_id: Optional[str] = None
        try:
            while True:
                raw = await ws.receive_json()
                try:
                    message = _adapter.validate_python(raw)
                except ValidationError as exc:
                    logger.warning("rejecting malformed plugin message: %s", exc)
                    continue

                if isinstance(message, Register):
                    instance_id = message.instanceId
                    await registry.upsert(message)
                    logger.info(
                        "registered instance %s (%s, %s)",
                        message.instanceId,
                        message.format,
                        message.role,
                    )
                elif isinstance(message, Metrics):
                    await registry.update_metrics(
                        message.instanceId,
                        message.metrics.model_dump(),
                        message.timestamp,
                    )
        except WebSocketDisconnect:
            pass
        finally:
            if instance_id is not None:
                await registry.mark_disconnected(instance_id)

    return router
