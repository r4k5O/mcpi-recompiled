import os
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
BUILD_WORKFLOW = ROOT / ".github" / "workflows" / "build.yml"
RELEASE_WORKFLOW = ROOT / ".github" / "workflows" / "release.yml"
SETUP_SCRIPT = ROOT / "setup.sh"
SETUP_TEST = ROOT / "tests" / "setup_script_tests.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def require_fragment(text: str, fragment: str, label: str) -> None:
    require(fragment in text, f"workflow missing {label}: {fragment!r}")


def main() -> int:
    require(BUILD_WORKFLOW.exists(), "build workflow must exist at .github/workflows/build.yml")
    require(RELEASE_WORKFLOW.exists(), "release workflow must exist at .github/workflows/release.yml")
    require(SETUP_SCRIPT.exists(), "automatic asset setup must exist at setup.sh")
    require(SETUP_TEST.exists(), "setup regression test must exist at tests/setup_script_tests.py")

    build = BUILD_WORKFLOW.read_text(encoding="utf-8")
    release = RELEASE_WORKFLOW.read_text(encoding="utf-8")
    setup = SETUP_SCRIPT.read_text(encoding="utf-8")

    release_fragments = {
        "version tag trigger": 'tags:\n      - "v*"',
        "manual release trigger": "workflow_dispatch:",
        "release type input": "release_type:",
        "patch release option": "- patch",
        "minor release option": "- minor",
        "major release option": "- major",
        "custom release option": "- custom",
        "custom version input": "custom_version:",
        "release permission": "contents: write",
        "release preparation job": "prepare:",
        "semantic tag discovery": "git tag --list 'v[0-9]*.[0-9]*.[0-9]*'",
        "safe sorted tag collection": "mapfile -t release_tags",
        "custom input precedence": 'if [[ -n "$CUSTOM_VERSION" ]]; then',
        "custom input forces custom mode": 'RELEASE_TYPE="custom"',
        "requested type log": 'echo "Requested release type: $RELEASE_TYPE"',
        "requested custom version log": 'echo "Requested custom version: ${CUSTOM_VERSION:-<empty>}"',
        "resolved tag log": 'echo "Resolved release tag: $tag"',
        "custom resolution guard": 'expected_custom="v${CUSTOM_VERSION#v}"',
        "custom mismatch failure": 'Custom release mismatch:',
        "source SHA output declaration": "source_sha: ${{ steps.version.outputs.source_sha }}",
        "source SHA resolution": 'source_sha="$(git rev-parse HEAD)"',
        "source SHA output": 'echo "source_sha=$source_sha" >> "$GITHUB_OUTPUT"',
        "build prepared source checkout": "ref: ${{ needs.prepare.outputs.source_sha }}",
        "Linux release runner": "ubuntu-latest",
        "Windows release runner": "windows-latest",
        "macOS ARM64 release runner": "macos-latest",
        "macOS x86-64 release runner": "macos-15-intel",
        "macOS ARM64 artifact": "release-macos-arm64",
        "macOS x86-64 artifact": "release-macos-x86_64",
        "Linux archive": "linux-x86_64.tar.gz",
        "Windows archive": "windows-x86_64.zip",
        "macOS ARM64 archive": "macos-arm64.tar.gz",
        "macOS x86-64 archive": "macos-x86_64.tar.gz",
        "ARM full-client job": "arm-build:",
        "ARM QEMU setup": "docker/setup-qemu-action@v4",
        "ARM QEMU platforms": "platforms: arm64,arm",
        "ARM stable runner": "runs-on: ubuntu-22.04",
        "ARM64 Docker platform": "docker_platform: linux/arm64",
        "ARM32 Docker platform": "docker_platform: linux/arm/v7",
        "ARM64 package suffix": "package_suffix: linux-arm64",
        "ARM32 package suffix": "package_suffix: linux-arm32",
        "ARM full client enabled": "-DMCPI_BUILD_CLIENT=ON",
        "ARM client binary check": 'test -x "build-arm/mcpi-recompiled"',
        "ARM executable architecture check": "file build-arm/mcpi-recompiled",
        "ARM64 release package": "linux-arm64.tar.gz",
        "ARM32 release package": "linux-arm32.tar.gz",
        "publish waits for ARM clients": "- arm-build",
        "checksums": "SHA256SUMS.txt",
        "generated notes": "generate_release_notes: true",
        "explicit release tag": "tag_name: ${{ needs.prepare.outputs.tag }}",
        "release target commit": "target_commitish: ${{ needs.prepare.outputs.source_sha }}",
        "Unix setup script packaging": "cp README.md LICENSE NOTICE LEGAL.md setup.sh",
        "Unix setup executable bit": 'chmod +x "dist/${package}/setup.sh"',
    }

    for label, fragment in release_fragments.items():
        require_fragment(release, fragment, label)

    require(
        release.count("cp README.md LICENSE NOTICE LEGAL.md setup.sh") >= 2,
        "setup.sh must be packaged in both native Unix and ARM release archives",
    )
    require(
        release.count('chmod +x "dist/${package}/setup.sh"') >= 2,
        "setup.sh must be executable in both native Unix and ARM release archives",
    )

    build_fragments = {
        "CI ARM full-client job": "arm-build:",
        "CI ARM QEMU setup": "docker/setup-qemu-action@v4",
        "CI ARM QEMU platforms": "platforms: arm64,arm",
        "CI ARM stable runner": "runs-on: ubuntu-22.04",
        "CI ARM64 Docker platform": "docker_platform: linux/arm64",
        "CI ARM32 Docker platform": "docker_platform: linux/arm/v7",
        "CI ARM client enabled": "-DMCPI_BUILD_CLIENT=ON",
        "CI ARM executable check": "file build-arm/mcpi-recompiled",
    }

    for label, fragment in build_fragments.items():
        require_fragment(build, fragment, label)

    # ARM builds run in a deliberately minimal target userland. Keep the SDL backend
    # set explicit so they do not depend on packages preinstalled on hosted x86 runners.
    arm_sdl_fragments = {
        "DRM development headers": "libdrm-dev",
        "GBM development headers": "libgbm-dev",
        "udev development headers": "libudev-dev",
        "dbus development headers": "libdbus-1-dev",
        "release build type": "-DCMAKE_BUILD_TYPE=Release",
        "X11 backend": "-DSDL_X11=ON",
        "ALSA backend": "-DSDL_ALSA=ON",
        "KMSDRM backend": "-DSDL_KMSDRM=ON",
        "udev backend": "-DSDL_LIBUDEV=ON",
        "dbus backend": "-DSDL_DBUS=ON",
        "Wayland disabled baseline": "-DSDL_WAYLAND=OFF",
        "PulseAudio disabled baseline": "-DSDL_PULSEAUDIO=OFF",
        "PipeWire disabled baseline": "-DSDL_PIPEWIRE=OFF",
        "JACK disabled baseline": "-DSDL_JACK=OFF",
        "sndio disabled baseline": "-DSDL_SNDIO=OFF",
        "libusb HID disabled baseline": "-DSDL_HIDAPI_LIBUSB=OFF",
        "IBus disabled baseline": "-DSDL_IBUS=OFF",
        "io_uring disabled baseline": "-DSDL_LIBURING=OFF",
        "legacy RPI backend disabled": "-DSDL_RPI=OFF",
    }
    for workflow_name, text in (("build", build), ("release", release)):
        for label, fragment in arm_sdl_fragments.items():
            require_fragment(text, fragment, f"{workflow_name} ARM {label}")

    # Manual releases must not publish a tag until build/test/package jobs have passed.
    require('git tag -a "$tag"' not in release, "prepare must not create the release tag")
    require('git push origin "$tag"' not in release, "prepare must not push the release tag")
    require("| head -n1" not in release, "tag discovery must not depend on a pipefail-sensitive head pipeline")

    # ARM release and CI jobs must build the real SDL client, not the old core-only smoke target.
    for workflow_name, text in (("build", build), ("release", release)):
        require("cross-build:" not in text, f"{workflow_name} workflow must replace ARM core-only cross-build with full client builds")
        require("-DMCPI_BUILD_CLIENT=OFF" not in text, f"{workflow_name} ARM builds must not disable the client")
        require("cmake/toolchains/linux-arm64.cmake" not in text,
                f"{workflow_name} ARM64 build must use a native ARM userland rather than the core-only cross toolchain")
        require("cmake/toolchains/linux-arm32.cmake" not in text,
                f"{workflow_name} ARM32 build must use a native ARM userland rather than the core-only cross toolchain")

    # Keep GitHub-hosted workflows on maintained Node 24-era action majors.
    for workflow_name, text in (("build", build), ("release", release)):
        require_fragment(text, "actions/checkout@v7", f"{workflow_name} checkout v7")
        require_fragment(text, "actions/setup-java@v5", f"{workflow_name} setup-java v5")
        require("actions/checkout@v4" not in text, f"{workflow_name} must not use checkout v4")
        require("actions/setup-java@v4" not in text, f"{workflow_name} must not use setup-java v4")

    require_fragment(release, "actions/upload-artifact@v7", "release upload-artifact v7")
    require_fragment(release, "actions/download-artifact@v8", "release download-artifact v8")
    require_fragment(release, "softprops/action-gh-release@v3", "release action-gh-release v3")

    for deprecated in (
        "actions/upload-artifact@v4",
        "actions/download-artifact@v4",
        "softprops/action-gh-release@v2",
    ):
        require(deprecated not in release, f"release workflow must not use deprecated action: {deprecated}")

    setup_fragments = {
        "official Minecraft download": "https://www.minecraft.net/content/dam/minecraftnet/games/minecraft/software/minecraft-pi-0.1.1.tar.gz.zip",
        "local archive override": "MCPI_ARCHIVE",
        "asset root override": "MCPI_ASSETS",
        "safe ZIP parser": "zipfile",
        "safe tar parser": "tarfile",
        "asset-only mode": "--assets-only",
        "forced reinstall mode": "--force",
    }
    for label, fragment in setup_fragments.items():
        require(fragment in setup, f"setup.sh missing {label}: {fragment!r}")

    # The functional extraction test uses bash, so run it on Unix CI while Windows
    # still enforces all static packaging/download contracts above.
    if os.name != "nt" and shutil.which("bash"):
        result = subprocess.run(
            [sys.executable, str(SETUP_TEST)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        require(result.returncode == 0, f"setup script functional regression failed:\n{result.stdout}")

    print("GitHub workflow/release contract passed with deterministic ARM builds and automatic Pi asset setup.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
