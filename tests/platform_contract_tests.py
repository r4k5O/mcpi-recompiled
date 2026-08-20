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
    platforms = (root / "docs" / "platforms.md")
    arm64 = root / "cmake" / "toolchains" / "linux-arm64.cmake"
    arm32 = root / "cmake" / "toolchains" / "linux-arm32.cmake"

    require("macos-latest" in workflow, "build workflow must include a macOS hosted job")
    require("arm64" in workflow, "build workflow must include an ARM64 configure/build job")
    require("arm32" in workflow or "armhf" in workflow, "build workflow must include an ARM32 configure/build job")
    require(arm64.is_file(), "linux-arm64.cmake must be checked in")
    require(arm32.is_file(), "linux-arm32.cmake must be checked in")
    require(platforms.is_file(), "docs/platforms.md must document supported build recipes")

    docs = platforms.read_text(encoding="utf-8").lower()
    require("macos" in docs and "arm64" in docs and "arm32" in docs, "platform docs must cover macOS, ARM64 and ARM32")
    require("cmake_toolchain_file" in docs, "platform docs must contain cross-toolchain commands")
    require("runtime" in docs and "build" in docs, "platform docs must distinguish build coverage from runtime claims")

    print("platform contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
