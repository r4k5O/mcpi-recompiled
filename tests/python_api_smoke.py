import math
import subprocess
import sys

from mcpi.minecraft import Minecraft


def fail(message: str) -> None:
    raise AssertionError(message)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: python_api_smoke.py <host-executable>", file=sys.stderr)
        return 2

    host = subprocess.Popen(
        [sys.argv[1]],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    try:
        assert host.stdout is not None
        port_line = host.stdout.readline().strip()
        if not port_line:
            stderr = host.stderr.read() if host.stderr is not None else ""
            fail(f"C++ host did not publish a port. stderr: {stderr}")

        port = int(port_line)
        mc = Minecraft.create("127.0.0.1", port)

        position = mc.player.getPos()
        if not (
            math.isclose(position.x, 1.5)
            and math.isclose(position.y, 2.0)
            and math.isclose(position.z, 3.25)
        ):
            fail(f"unexpected player position: {position}")

        mc.setBlock(1, 2, 3, 57)
        stored_block = mc.getBlock(1, 2, 3)
        if stored_block != 57:
            fail(f"setBlock/getBlock round trip returned {stored_block}, expected 57")

        mc.postToChat("Hello from Python")

        return_code = host.wait(timeout=10)
        if return_code != 0:
            stderr = host.stderr.read() if host.stderr is not None else ""
            fail(f"C++ host exited with {return_code}. stderr: {stderr}")

        print("mcpi Python API smoke test passed against reconstructed GameState.")
        return 0
    finally:
        if host.poll() is None:
            host.kill()
            host.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
