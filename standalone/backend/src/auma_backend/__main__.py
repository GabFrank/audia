"""Entry point for `python -m auma_backend` and the PyInstaller binary."""

from __future__ import annotations

import logging

import uvicorn

from .app import create_app
from .config import Settings


def main() -> None:
    settings = Settings.from_env()
    logging.basicConfig(
        level=settings.log_level.upper(),
        format="%(asctime)s %(levelname)s %(name)s — %(message)s",
    )
    # Force the asyncio loop instead of uvloop. uvloop is a Cython
    # extension that hangs at import inside PyInstaller --onefile bundles
    # on macOS; asyncio is fast enough for the loopback traffic this
    # sidecar handles.
    uvicorn.run(
        create_app(settings),
        host=settings.host,
        port=settings.port,
        log_level=settings.log_level,
        loop="asyncio",
        http="h11",
        ws="websockets",
    )


if __name__ == "__main__":
    main()
