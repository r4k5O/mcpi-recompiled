import json
import pathlib
import subprocess
import sys
import tempfile


def find_cli(build_dir: pathlib.Path) -> pathlib.Path:
    candidates = [
        build_dir / "mcpi-parity",
        build_dir / "mcpi-parity.exe",
        build_dir / "Release" / "mcpi-parity.exe",
        build_dir / "Debug" / "mcpi-parity.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise AssertionError(f"mcpi-parity executable not found under {build_dir}")


def main() -> int:
    if len(sys.argv) != 3:
        raise AssertionError("usage: parity_cli_tests.py <build-dir> <reference-path>")

    build_dir = pathlib.Path(sys.argv[1])
    reference_path = pathlib.Path(sys.argv[2])
    cli = find_cli(build_dir)

    with tempfile.TemporaryDirectory() as tmp:
        json_path = pathlib.Path(tmp) / "report.json"
        result = subprocess.run(
            [
                str(cli),
                "--suite",
                "api",
                "--references",
                str(reference_path),
                "--json",
                str(json_path),
            ],
            text=True,
            capture_output=True,
            check=False,
        )
        assert result.returncode == 0, result.stderr
        assert "Loaded 1 reference case" in result.stdout
        assert "world-get-block-origin" in result.stdout

        report = json.loads(json_path.read_text(encoding="utf-8"))
        assert report["suite"] == "api"
        assert report["loaded"] == 1
        assert report["evaluated"] == 0
        assert report["matched"] == 0
        assert report["failed"] == 0
        assert len(report["cases"]) == 1
        assert report["cases"][0]["name"] == "world-get-block-origin"
        assert report["cases"][0]["status"] == "pending"

        invalid = subprocess.run(
            [str(cli), "--suite", "api"],
            text=True,
            capture_output=True,
            check=False,
        )
        assert invalid.returncode != 0
        assert "--references" in invalid.stderr

    print("mcpi-parity CLI contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
