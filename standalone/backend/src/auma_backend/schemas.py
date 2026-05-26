"""Pydantic mirrors of the JSON Schemas under /shared/protocol.

These models are the canonical Python representation of the wire
protocol. They are intentionally permissive on the `metrics` payload
so new fields can ride along without a version bump.
"""

from __future__ import annotations

from typing import Annotated, Any, Literal, Optional, Union

from pydantic import BaseModel, ConfigDict, Field

Format = Literal["AU", "VST3", "Standalone"]
Role = Literal["track", "bus", "master"]
CommandName = Literal["identify", "startCapture", "setReference", "setRole"]


class Register(BaseModel):
    model_config = ConfigDict(extra="forbid")

    type: Literal["register"]
    protocolVersion: Optional[str] = None
    instanceId: str = Field(min_length=1)
    format: Format
    role: Role
    trackName: Optional[str] = None
    sampleRate: float = Field(gt=0)
    channels: int = Field(ge=1, le=8)
    isFirstInChain: Optional[bool] = None
    pluginVersion: Optional[str] = None


class MetricsPayload(BaseModel):
    model_config = ConfigDict(extra="allow")

    lufsMomentary: Optional[float] = None


class Metrics(BaseModel):
    model_config = ConfigDict(extra="forbid")

    type: Literal["metrics"]
    instanceId: str = Field(min_length=1)
    timestamp: float
    metrics: MetricsPayload


class Command(BaseModel):
    model_config = ConfigDict(extra="forbid")

    type: Literal["command"]
    command: CommandName
    targetInstanceId: Optional[str] = None
    args: Optional[dict[str, Any]] = None


PluginMessage = Annotated[
    Union[Register, Metrics],
    Field(discriminator="type"),
]


class InstanceSnapshot(BaseModel):
    """Server-side view of a single connected plugin instance."""

    instanceId: str
    format: Format
    role: Role
    trackName: Optional[str] = None
    sampleRate: float
    channels: int
    isFirstInChain: Optional[bool] = None
    pluginVersion: Optional[str] = None
    connected: bool
    lastSeen: float
    metrics: dict[str, Any] = Field(default_factory=dict)


class RegistryEvent(BaseModel):
    """Frame pushed to /ws/frontend whenever the registry changes."""

    type: Literal["snapshot", "instance", "instanceRemoved"]
    instances: Optional[list[InstanceSnapshot]] = None
    instance: Optional[InstanceSnapshot] = None
    instanceId: Optional[str] = None
