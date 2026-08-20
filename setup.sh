#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
OFFICIAL_PI_URL="https://www.minecraft.net/content/dam/minecraftnet/games/minecraft/software/minecraft-pi-0.1.1.tar.gz.zip"
ASSET_DIR="${MCPI_ASSETS:-${SCRIPT_DIR}/mcpi}"
ARCHIVE_SOURCE="${MCPI_ARCHIVE:-}"
FORCE=0
ASSETS_ONLY=0
FORWARD_ARGS=()
WORK_DIR=""

log() {
  printf '[mcpi-setup] %s\n' "$*"
}

fail() {
  printf '[mcpi-setup] ERROR: %s\n' "$*" >&2
  exit 1
}

usage() {
  cat <<'EOF'
mcpi-recompiled setup

Downloads the original, freely released Minecraft: Pi Edition 0.1.1 archive
from the official Minecraft website and installs it as the local asset source.
The original game is downloaded at setup time and is not redistributed by this
repository or by mcpi-recompiled release archives.

Usage:
  bash setup.sh [options] [-- mcpi-recompiled arguments...]

Options:
  --assets-only   Install/update the original Pi Edition files, then exit.
  --force         Reinstall even if the asset directory already exists.
  -h, --help      Show this help.

Environment:
  MCPI_ASSETS     Destination asset root. Default: ./mcpi next to setup.sh
  MCPI_ARCHIVE    Use an already-downloaded local Pi Edition archive instead
                  of downloading from minecraft.net. Useful for offline setup.

Examples:
  bash setup.sh
  bash setup.sh --assets-only
  bash setup.sh --force
  MCPI_ASSETS="$HOME/minecraft-pi" bash setup.sh
  MCPI_ARCHIVE="$HOME/Downloads/minecraft-pi-0.1.1.tar.gz.zip" bash setup.sh
EOF
}

cleanup() {
  if [[ -n "${WORK_DIR}" && -d "${WORK_DIR}" ]]; then
    rm -rf -- "${WORK_DIR}"
  fi
}
trap cleanup EXIT INT TERM

while (($#)); do
  case "$1" in
    --assets-only)
      ASSETS_ONLY=1
      shift
      ;;
    --force)
      FORCE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      FORWARD_ARGS=("$@")
      break
      ;;
    *)
      fail "Unknown setup option: $1 (use --help)"
      ;;
  esac
done

command -v python3 >/dev/null 2>&1 || fail "python3 is required for safe archive extraction."

if [[ -d "${ASSET_DIR}" && "${FORCE}" -eq 0 ]]; then
  log "Asset directory already exists: ${ASSET_DIR}"
  log "Use --force to reinstall it."
else
  if [[ "${FORCE}" -eq 1 && -e "${ASSET_DIR}" ]]; then
    resolved_assets="$(python3 - "${ASSET_DIR}" <<'PY'
import os
import sys
print(os.path.realpath(sys.argv[1]))
PY
)"
    resolved_script="$(python3 - "${SCRIPT_DIR}" <<'PY'
import os
import sys
print(os.path.realpath(sys.argv[1]))
PY
)"
    resolved_home="$(python3 - <<'PY'
import os
print(os.path.realpath(os.path.expanduser('~')))
PY
)"

    case "${resolved_assets}" in
      /|"${resolved_script}"|"${resolved_home}")
        fail "Refusing to remove unsafe asset destination: ${resolved_assets}"
        ;;
    esac
    log "Removing existing asset directory: ${ASSET_DIR}"
    rm -rf -- "${ASSET_DIR}"
  fi

  WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/mcpi-recompiled-setup.XXXXXX")"
  DOWNLOAD_FILE="${WORK_DIR}/minecraft-pi-download"

  if [[ -n "${ARCHIVE_SOURCE}" ]]; then
    [[ -f "${ARCHIVE_SOURCE}" ]] || fail "MCPI_ARCHIVE does not exist: ${ARCHIVE_SOURCE}"
    log "Using local Minecraft Pi archive: ${ARCHIVE_SOURCE}"
    cp -- "${ARCHIVE_SOURCE}" "${DOWNLOAD_FILE}"
  else
    log "Downloading Minecraft: Pi Edition 0.1.1 from the official Minecraft website..."
    log "Source: ${OFFICIAL_PI_URL}"

    if command -v curl >/dev/null 2>&1; then
      curl \
        --fail \
        --location \
        --show-error \
        --silent \
        --retry 3 \
        --proto '=https' \
        --tlsv1.2 \
        --output "${DOWNLOAD_FILE}" \
        "${OFFICIAL_PI_URL}" || fail "Official Minecraft Pi download failed."
    elif command -v wget >/dev/null 2>&1; then
      wget \
        --quiet \
        --https-only \
        --max-redirect=10 \
        --output-document="${DOWNLOAD_FILE}" \
        "${OFFICIAL_PI_URL}" || fail "Official Minecraft Pi download failed."
    else
      fail "Either curl or wget is required to download Minecraft Pi."
    fi
  fi

  [[ -s "${DOWNLOAD_FILE}" ]] || fail "Downloaded archive is empty."

  log "Safely extracting original Minecraft Pi files..."
  python3 - "${DOWNLOAD_FILE}" "${WORK_DIR}/extract" "${ASSET_DIR}" <<'PY'
from __future__ import annotations

import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import sys
import tarfile
import zipfile

archive_path = Path(sys.argv[1])
extract_root = Path(sys.argv[2])
destination = Path(sys.argv[3])


def fail(message: str) -> None:
    raise SystemExit(f"[mcpi-setup] ERROR: {message}")


def validate_name(name: str) -> None:
    # Never allow path traversal or absolute archive members.
    normalized = name.replace("\\", "/")
    path = PurePosixPath(normalized)
    if path.is_absolute() or normalized.startswith("/") or ".." in path.parts:
        fail(f"archive path traversal rejected: {name!r}")
    if not path.parts:
        fail("archive contains an empty member name")


def extract_zip(source: Path, target: Path) -> None:
    target.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(source) as archive:
        for info in archive.infolist():
            validate_name(info.filename)
            mode = (info.external_attr >> 16) & 0o170000
            if mode and stat.S_ISLNK(mode):
                fail(f"archive symbolic link rejected: {info.filename!r}")
        archive.extractall(target)


def extract_tar(source: Path, target: Path) -> None:
    target.mkdir(parents=True, exist_ok=True)
    with tarfile.open(source, mode="r:*") as archive:
        members = archive.getmembers()
        for member in members:
            validate_name(member.name)
            if member.issym() or member.islnk() or member.isdev():
                fail(f"archive link/device member rejected: {member.name!r}")
        archive.extractall(target, members=members)


outer = extract_root / "outer"
payload = extract_root / "payload"

if zipfile.is_zipfile(archive_path):
    extract_zip(archive_path, outer)
    nested_tarballs = [
        path
        for path in outer.rglob("*")
        if path.is_file() and tarfile.is_tarfile(path)
    ]
    if nested_tarballs:
        preferred = [p for p in nested_tarballs if p.name == "minecraft-pi-0.1.1.tar.gz"]
        nested = preferred[0] if preferred else nested_tarballs[0]
        extract_tar(nested, payload)
        source_root = payload
    else:
        source_root = outer
elif tarfile.is_tarfile(archive_path):
    extract_tar(archive_path, payload)
    source_root = payload
else:
    fail("download is neither a valid ZIP nor a valid tar archive")

candidates = []
direct = source_root / "mcpi"
if direct.is_dir():
    candidates.append(direct)
for candidate in source_root.rglob("mcpi"):
    if candidate.is_dir() and candidate not in candidates:
        candidates.append(candidate)

if not candidates:
    fail("archive does not contain the expected top-level mcpi directory")

source = candidates[0]
if destination.exists():
    shutil.rmtree(destination)
destination.parent.mkdir(parents=True, exist_ok=True)
shutil.copytree(source, destination, symlinks=False)

files = [path for path in destination.rglob("*") if path.is_file()]
if not files:
    shutil.rmtree(destination, ignore_errors=True)
    fail("extracted mcpi directory contains no files")

print(f"[mcpi-setup] Installed {len(files)} original Pi Edition files to {destination}")
PY

  log "Minecraft Pi asset setup complete."
fi

if [[ "${ASSETS_ONLY}" -eq 1 ]]; then
  exit 0
fi

BINARY=""
for candidate in \
  "${SCRIPT_DIR}/mcpi-recompiled" \
  "${SCRIPT_DIR}/build/mcpi-recompiled" \
  "${SCRIPT_DIR}/build/Release/mcpi-recompiled"
do
  if [[ -x "${candidate}" ]]; then
    BINARY="${candidate}"
    break
  fi
done

if [[ -z "${BINARY}" ]]; then
  log "Assets are installed, but no mcpi-recompiled executable was found next to setup.sh or in ./build."
  log "Run the downloaded release binary manually with: --assets \"${ASSET_DIR}\""
  exit 0
fi

log "Starting mcpi-recompiled with original Pi Edition assets..."
exec "${BINARY}" --assets "${ASSET_DIR}" "${FORWARD_ARGS[@]}"
