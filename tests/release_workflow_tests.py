from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD_WORKFLOW = ROOT / ".github" / "workflows" / "build.yml"
RELEASE_WORKFLOW = ROOT / ".github" / "workflows" / "release.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def require_fragment(text: str, fragment: str, label: str) -> None:
    require(fragment in text, f"workflow missing {label}: {fragment!r}")


def main() -> int:
    require(BUILD_WORKFLOW.exists(), "build workflow must exist at .github/workflows/build.yml")
    require(RELEASE_WORKFLOW.exists(), "release workflow must exist at .github/workflows/release.yml")

    build = BUILD_WORKFLOW.read_text(encoding="utf-8")
    release = RELEASE_WORKFLOW.read_text(encoding="utf-8")

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
    }

    for label, fragment in release_fragments.items():
        require_fragment(release, fragment, label)

    # Manual releases must not publish a tag until build/test/package jobs have passed.
    require('git tag -a "$tag"' not in release, "prepare must not create the release tag")
    require('git push origin "$tag"' not in release, "prepare must not push the release tag")
    require("| head -n1" not in release, "tag discovery must not depend on a pipefail-sensitive head pipeline")

    # ARM release jobs must build the real SDL client, not the old core-only smoke target.
    require("cross-build:" not in release, "release workflow must replace ARM core-only cross-build with full client builds")
    require("-DMCPI_BUILD_CLIENT=OFF" not in release, "release ARM builds must not disable the client")
    require("cmake/toolchains/linux-arm64.cmake" not in release,
            "release ARM64 build must use a native ARM userland rather than the core-only cross toolchain")
    require("cmake/toolchains/linux-arm32.cmake" not in release,
            "release ARM32 build must use a native ARM userland rather than the core-only cross toolchain")

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

    print("GitHub workflow/release contract passed with six full native client release packages, including ARM64 and ARM32.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
