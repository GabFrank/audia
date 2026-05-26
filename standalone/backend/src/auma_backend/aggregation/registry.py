"""In-memory registry of connected plugin instances.

This is deliberately ephemeral. Persistence (SQLite per the design
document) is deferred until the project enters the versioning phase.
The registry is also the pub/sub hub: the frontend WebSocket layer
subscribes here and forwards events to UI clients.
"""

from __future__ import annotations

import asyncio
import time
from typing import Any

from ..schemas import InstanceSnapshot, Register, RegistryEvent


class InstanceRegistry:
    def __init__(self) -> None:
        self._instances: dict[str, InstanceSnapshot] = {}
        self._subscribers: set[asyncio.Queue[RegistryEvent]] = set()
        self._lock = asyncio.Lock()

    async def upsert(self, payload: Register) -> InstanceSnapshot:
        async with self._lock:
            existing = self._instances.get(payload.instanceId)
            snapshot = InstanceSnapshot(
                instanceId=payload.instanceId,
                format=payload.format,
                role=payload.role,
                trackName=payload.trackName,
                sampleRate=payload.sampleRate,
                channels=payload.channels,
                isFirstInChain=payload.isFirstInChain,
                pluginVersion=payload.pluginVersion,
                connected=True,
                lastSeen=time.time(),
                metrics=existing.metrics if existing else {},
            )
            self._instances[payload.instanceId] = snapshot
        await self._broadcast(RegistryEvent(type="instance", instance=snapshot))
        return snapshot

    async def update_metrics(
        self, instance_id: str, metrics: dict[str, Any], timestamp: float
    ) -> InstanceSnapshot | None:
        async with self._lock:
            snapshot = self._instances.get(instance_id)
            if snapshot is None:
                return None
            snapshot.metrics.update(metrics)
            snapshot.lastSeen = timestamp / 1000.0 if timestamp > 1e12 else time.time()
        await self._broadcast(RegistryEvent(type="instance", instance=snapshot))
        return snapshot

    async def mark_disconnected(self, instance_id: str) -> None:
        async with self._lock:
            snapshot = self._instances.get(instance_id)
            if snapshot is None:
                return
            snapshot.connected = False
        await self._broadcast(RegistryEvent(type="instance", instance=snapshot))

    async def snapshot(self) -> list[InstanceSnapshot]:
        async with self._lock:
            return list(self._instances.values())

    async def subscribe(self) -> asyncio.Queue[RegistryEvent]:
        queue: asyncio.Queue[RegistryEvent] = asyncio.Queue(maxsize=256)
        self._subscribers.add(queue)
        return queue

    def unsubscribe(self, queue: asyncio.Queue[RegistryEvent]) -> None:
        self._subscribers.discard(queue)

    async def _broadcast(self, event: RegistryEvent) -> None:
        for queue in list(self._subscribers):
            try:
                queue.put_nowait(event)
            except asyncio.QueueFull:
                # Slow consumer — drop the event rather than blocking the
                # plugin path. The next snapshot the consumer requests
                # will repair its view.
                pass
