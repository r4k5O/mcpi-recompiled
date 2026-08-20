import subprocess
import sys
import tempfile
from pathlib import Path


def fail(message: str) -> None:
    raise AssertionError(message)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: assets_cli_smoke.py <mcpi-recompiled-executable>", file=sys.stderr)
        return 2

    executable = sys.argv[1]

    missing = subprocess.run(
        [executable, "--assets"],
        capture_output=True,
        text=True,
        timeout=10,
        check=False,
    )
    if missing.returncode != 2 or "Missing --assets path." not in missing.stderr:
        fail(
            "missing --assets value must be rejected with exit code 2; "
            f"got code={missing.returncode}, stderr={missing.stderr!r}"
        )

    with tempfile.TemporaryDirectory(prefix="mcpi-assets-cli-") as temporary:
        root = Path(temporary)
        assets = root / "pi-assets"
        assets.mkdir()
        (assets / "sentinel.txt").write_text("local-only", encoding="utf-8")
        world = root / "world.mcpiworld"

        process = subprocess.Popen(
            [
                executable,
                "--headless",
                "--port",
                "0",
                "--world",
                str(world),
                "--assets",
                str(assets),
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        try:
            assert process.stdout is not None
            first_line = process.stdout.readline().strip()
            if not first_line.startswith("MCPI API listening on port "):
                stderr = process.stderr.read() if process.poll() is not None and process.stderr is not None else ""
                fail(f"explicit --assets path did not reach runtime startup: {first_line!r}; stderr={stderr!r}")
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)

    print("--assets CLI contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
