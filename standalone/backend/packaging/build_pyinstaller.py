"""Freeze auma-backend into a single executable for the host platform.

Invocation:
    python packaging/build_pyinstaller.py

Produces:
    standalone/backend/dist/auma-backend(.exe)

Cross-architecture builds are not attempted; run this on the target
machine. The Electron app looks for the binary at the path above when
launching the sidecar in production builds.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DIST = ROOT / "dist"
BUILD = ROOT / "build"
ENTRY = ROOT / "packaging" / "entrypoint.py"
SRC = ROOT / "src"
NAME = "auma-backend"


def main() -> None:
    if not ENTRY.is_file():
        sys.exit(f"entry point not found: {ENTRY}")

    if DIST.exists():
        shutil.rmtree(DIST)
    if BUILD.exists():
        shutil.rmtree(BUILD)

    cmd = [
        sys.executable,
        "-m",
        "PyInstaller",
        "--noconfirm",
        "--clean",
        # --onedir (default) lays out a directory bundle. We avoid --onefile
        # because its runtime self-extraction triggers macOS Gatekeeper
        # checks on every dlopen, which hang on networks where Apple's
        # OCSP/notary endpoints are unreachable. The Electron app ships the
        # whole directory anyway.
        "--name",
        NAME,
        "--distpath",
        str(DIST),
        "--workpath",
        str(BUILD),
        "--specpath",
        str(BUILD),
        "--paths",
        str(SRC),
        # uvicorn picks workers/loops/HTTP impl dynamically; help PyInstaller see them.
        "--collect-submodules",
        "uvicorn",
        "--hidden-import",
        "uvicorn.loops.auto",
        "--hidden-import",
        "uvicorn.protocols.http.auto",
        "--hidden-import",
        "uvicorn.protocols.websockets.auto",
        "--hidden-import",
        "uvicorn.lifespan.on",
        # Belt-and-braces: these are not installed when [standard] extras are
        # omitted, but if they appear in a developer's env we still want the
        # frozen build to ignore them.
        "--exclude-module",
        "uvloop",
        "--exclude-module",
        "watchfiles",
        "--exclude-module",
        "httptools",
        str(ENTRY),
    ]
    print("running:", " ".join(cmd))
    subprocess.check_call(cmd, cwd=ROOT)
    binary = DIST / NAME
    if not binary.exists() and sys.platform == "win32":
        binary = DIST / f"{NAME}.exe"
    print(f"\nbuilt: {binary}")


if __name__ == "__main__":
    main()
