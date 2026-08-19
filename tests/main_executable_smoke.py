import math
import subprocess
import sys

from mcpi.minecraft import Minecraft


def fail(message: str) -> None:
    raise AssertionError(message)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: main_executable_smoke.py <mcpi-recompiled-executable>", file=sys.stderr)
        return 2

    process = subprocess.Popen(
        [sys.argv[1], "--headless", "--port", "0"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    try:
        assert process.stdout is not None
        first_line = process.stdout.readline().strip()
        prefix = "MCPI API listening on port "
        if not first_line.startswith(prefix):
            stderr = process.stderr.read() if process.poll() is not None and process.stderr is not None else ""
            fail(f"main executable did not publish its API port: {first_line!r}. stderr: {stderr}")

        port = int(first_line[len(prefix):])
        if port <= 0:
            fail(f"main executable published invalid port {port}")

        mc = Minecraft.create("127.0.0.1", port)

        mc.player.setPos(10.5, 20.0, -3.25)
        position = mc.player.getPos()
        if not (
            math.isclose(position.x, 10.5)
            and math.isclose(position.y, 20.0)
            and math.isclose(position.z, -3.25)
        ):
            fail(f"player position did not round-trip through main executable: {position}")

        mc.setBlock(4, 5, 6, 57)
        if mc.getBlock(4, 5, 6) != 57:
            fail("block did not round-trip through main executable GameState")

        mc.postToChat("Hello main executable")
        print("main executable MCPI API smoke test passed.")
        return 0
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
