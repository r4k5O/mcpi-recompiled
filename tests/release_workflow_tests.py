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
        "Linux archive": "linux-x86_64.tar.gz",
        "Windows archive": "windows_x86_64.zip",
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

    print("GitHub workflow/release contract passed with post-build tag publication and maintained actions.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
