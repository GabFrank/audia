"""FastAPI application factory."""

from __future__ import annotations

import logging

from fastapi import FastAPI

from . import __version__
from .aggregation.registry import InstanceRegistry
from .config import Settings
from .server.frontend_ws import build_router as build_frontend_router
from .server.plugin_ws import build_router as build_plugin_router

logger = logging.getLogger(__name__)


def create_app(settings: Settings | None = None) -> FastAPI:
    settings = settings or Settings.from_env()
    app = FastAPI(title="AuMA backend", version=__version__)
    registry = InstanceRegistry()
    app.state.settings = settings
    app.state.registry = registry

    @app.get("/healthz")
    async def healthz() -> dict[str, str]:
        return {"status": "ok", "version": __version__}

    @app.get("/state")
    async def state() -> dict[str, object]:
        return {"instances": [s.model_dump() for s in await registry.snapshot()]}

    app.include_router(build_plugin_router(registry))
    app.include_router(build_frontend_router(registry))
    return app
