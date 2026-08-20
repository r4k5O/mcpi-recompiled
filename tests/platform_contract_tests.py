from pathlib import Path
import sys


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: platform_contract_tests.py <repo-root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    workflow = (root / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8").lower()
    platforms = root / "docs" / "platforms.md"
    cmake_lists = root / "CMakeLists.txt"
    arm64 = root / "cmake" / "toolchains" / "linux-arm64.cmake"
    arm32 = root / "cmake" / "toolchains" / "linux-arm32.cmake"

    require("macos-latest" in workflow, "build workflow must include a macOS hosted job")
    require("arm-build:" in workflow, "build workflow must include full ARM client jobs")
    require("docker/setup-qemu-action@v4" in workflow, "ARM client jobs must configure QEMU")
    require("linux/arm64" in workflow, "build workflow must include the Linux ARM64 target")
    require("linux/arm/v7" in workflow, "build workflow must include the Linux ARMv7 target")
    require("-dmcpi_build_client=on" in workflow, "ARM jobs must build the full client")

    require(arm64.is_file(), "linux-arm64.cmake must remain available for developer cross-build experiments")
    require(arm32.is_file(), "linux-arm32.cmake must remain available for developer cross-build experiments")
    require(platforms.is_file(), "docs/platforms.md must document supported build recipes")
    require(cmake_lists.is_file(), "CMakeLists.txt must exist")

    docs = platforms.read_text(encoding="utf-8").lower()
    require("macos" in docs and "arm64" in docs and "arm32" in docs,
            "platform docs must cover macOS, ARM64 and ARM32")
    require("qemu" in docs and "linux/arm64" in docs and "linux/arm/v7" in docs,
            "platform docs must describe the full QEMU ARM client targets")
    require("armv6" in docs and "raspberry pi 1" in docs,
            "platform docs must not overclaim ARMv7 as Raspberry Pi 1 compatibility")
    require("runtime" in docs and "build" in docs,
            "platform docs must distinguish build coverage from runtime claims")

    cmake = cmake_lists.read_text(encoding="utf-8").lower()
    require("cmake_minimum_required(version 3.20)" in cmake,
            "project must retain its declared CMake 3.20 baseline")
    require('if(cmake_version version_greater_equal "3.24")' in cmake,
            "SDL FetchContent timestamp option must be gated for pre-3.24 CMake")
    require("download_extract_timestamp true" in cmake,
            "newer CMake must keep the explicit SDL extraction timestamp behavior")

    print("platform contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
