from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github" / "workflows" / "release.yml"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    require(WORKFLOW.exists(), "release workflow must exist at .github/workflows/release.yml")
    text = WORKFLOW.read_text(encoding="utf-8")

    required_fragments = {
        "version tag trigger": 'tags:\n      - "v*"',
        "release permission": "contents: write",
        "Linux release runner": "ubuntu-latest",
        "Windows release runner": "windows-latest",
        "Linux archive": "linux-x86_64.tar.gz",
        "Windows archive": "windows-x86_64.zip",
        "checksums": "SHA256SUMS.txt",
        "release publication": "softprops/action-gh-release@",
        "generated notes": "generate_release_notes: true",
    }

    for label, fragment in required_fragments.items():
        require(fragment in text, f"release workflow missing {label}: {fragment!r}")

    print("GitHub release workflow contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
