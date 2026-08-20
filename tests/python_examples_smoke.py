import subprocess
import sys

from mcpi import block
from mcpi.minecraft import Minecraft
from mcpi.vec3 import Vec3


def main():
    if len(sys.argv) != 2:
        return 2
    host = subprocess.Popen([sys.argv[1]], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert host.stdout is not None
        port = int(host.stdout.readline().strip())
        mc = Minecraft.create("127.0.0.1", port)

        # Headless equivalents of common bundled-demo command patterns.
        origin = Vec3(10, 5, 10)
        mc.setBlocks(origin.x, origin.y, origin.z, origin.x + 2, origin.y, origin.z, block.STONE.id)
        for dx in range(3):
            if mc.getBlock(origin.x + dx, origin.y, origin.z) != block.STONE.id:
                raise AssertionError("block run mismatch")
        mc.player.setTilePos(origin)
        mc.camera.setFollow(0)
        mc.saveCheckpoint()
        mc.restoreCheckpoint()
        mc.postToChat("__mcpi_python_done__")
        code = host.wait(timeout=10)
        if code != 0:
            stderr = host.stderr.read() if host.stderr is not None else ""
            raise AssertionError(stderr)
        return 0
    finally:
        if host.poll() is None:
            host.kill()
            host.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
