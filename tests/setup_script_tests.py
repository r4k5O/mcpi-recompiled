from __future__ import annotations

import io
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile
import zipfile


ROOT = Path(__file__).resolve().parents[1]
SETUP = ROOT / "setup.sh"
OFFICIAL_URL = (
    "https://www.minecraft.net/content/dam/minecraftnet/games/"
    "minecraft/software/minecraft-pi-0.1.1.tar.gz.zip"
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def make_reference_archive(path: Path) -> None:
    tar_bytes = io.BytesIO()
    with tarfile.open(fileobj=tar_bytes, mode="w:gz") as archive:
        payloads = {
            "mcpi/data/images/terrain.png": b"not-a-real-minecraft-texture\n",
            "mcpi/api/python/mcpi/__init__.py": b"# synthetic setup test\n",
        }
        for name, payload in payloads.items():
            info = tarfile.TarInfo(name)
            info.size = len(payload)
            info.mode = 0o644
            archive.addfile(info, io.BytesIO(payload))

    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as outer:
        outer.writestr("minecraft-pi-0.1.1.tar.gz", tar_bytes.getvalue())


def make_traversal_archive(path: Path) -> None:
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("../escaped.txt", "must not escape extraction root")


def run_setup(archive: Path, assets: Path, *args: str) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["MCPI_ARCHIVE"] = str(archive)
    env["MCPI_ASSETS"] = str(assets)
    return subprocess.run(
        ["bash", str(SETUP), "--assets-only", *args],
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )


def main() -> int:
    require(SETUP.exists(), "setup.sh must exist at repository root")
    text = SETUP.read_text(encoding="utf-8")

    for fragment, label in (
        (OFFICIAL_URL, "official Minecraft Pi download URL"),
        ("MCPI_ARCHIVE", "offline/local archive override"),
        ("MCPI_ASSETS", "asset destination override"),
        ("--assets-only", "asset-only mode"),
        ("--force", "forced reinstall mode"),
        ("curl", "curl downloader"),
        ("wget", "wget downloader fallback"),
        ("zipfile", "ZIP extraction"),
        ("tarfile", "tar extraction"),
        ("path traversal", "archive traversal guard"),
    ):
        require(fragment in text, f"setup.sh missing {label}: {fragment!r}")

    with tempfile.TemporaryDirectory(prefix="mcpi-setup-test-") as temporary:
        temp = Path(temporary)
        archive = temp / "minecraft-pi.zip"
        assets = temp / "installed-mcpi"
        make_reference_archive(archive)

        first = run_setup(archive, assets)
        require(first.returncode == 0, f"setup failed for valid reference archive:\n{first.stdout}")
        require((assets / "data/images/terrain.png").is_file(), "setup did not install mcpi data tree")
        require((assets / "api/python/mcpi/__init__.py").is_file(), "setup did not install mcpi API tree")

        second = run_setup(archive, assets)
        require(second.returncode == 0, f"idempotent setup rerun failed:\n{second.stdout}")
        require("already exists" in second.stdout.lower(), "rerun should report existing asset directory")

        forced = run_setup(archive, assets, "--force")
        require(forced.returncode == 0, f"forced reinstall failed:\n{forced.stdout}")

        malicious = temp / "malicious.zip"
        malicious_assets = temp / "malicious-assets"
        escaped = temp / "escaped.txt"
        make_traversal_archive(malicious)
        bad = run_setup(malicious, malicious_assets)
        require(bad.returncode != 0, "path-traversal archive must be rejected")
        require(not escaped.exists(), "malicious archive escaped the extraction directory")

    print("Automatic Minecraft Pi asset setup contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
