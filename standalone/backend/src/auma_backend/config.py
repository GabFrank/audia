"""Runtime configuration."""

from __future__ import annotations

import os
from dataclasses import dataclass


@dataclass(frozen=True)
class Settings:
    host: str = "127.0.0.1"
    port: int = 17600
    log_level: str = "info"

    @classmethod
    def from_env(cls) -> "Settings":
        return cls(
            host=os.environ.get("AUMA_HOST", "127.0.0.1"),
            port=int(os.environ.get("AUMA_PORT", "17600")),
            log_level=os.environ.get("AUMA_LOG_LEVEL", "info"),
        )
