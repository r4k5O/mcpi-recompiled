import math
import subprocess
import sys

from mcpi import block
from mcpi.minecraft import Minecraft
from mcpi.vec3 import Vec3


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    if len(sys.argv) != 2:
        return 2
    host = subprocess.Popen([sys.argv[1]], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert host.stdout is not None
        port = int(host.stdout.readline().strip())
        mc = Minecraft.create("127.0.0.1", port)

        p = mc.player.getPos()
        require(math.isclose(p.x, 1.5) and math.isclose(p.y, 2.0) and math.isclose(p.z, 3.25), "initial pos")
        mc.player.setPos(Vec3(-0.25, 4.5, 2.75))
        p = mc.player.getPos()
        require(math.isclose(p.x, -0.25) and math.isclose(p.y, 4.5) and math.isclose(p.z, 2.75), "setPos")
        mc.player.setTilePos(2, 3, 4)
        require(tuple(mc.player.getTilePos()) == (2, 3, 4), "tile position")

        mc.setBlock(1, 1, 1, block.WOOL.id, 5)
        with_data = mc.getBlockWithData(1, 1, 1)
        require(with_data.id == block.WOOL.id and with_data.data == 5, "block data")

        mc.setBlocks(0, 0, 0, 1, 0, 0, block.STONE.id)
        blocks = list(mc.getBlocks(0, 0, 0, 1, 0, 0))
        require(blocks == [block.STONE.id, block.STONE.id], f"getBlocks order/content: {blocks}")
        require(mc.getHeight(0, 0) >= 0, "height")

        ids = mc.getPlayerEntityIds()
        require(ids == [0], f"player ids: {ids}")
        mc.entity.setPos(0, 5, 6, 7)
        ep = mc.entity.getPos(0)
        require(tuple(ep) == (5.0, 6.0, 7.0), f"entity pos: {ep}")

        mc.camera.setNormal(0)
        mc.camera.setFollow(0)
        mc.camera.setFixed()
        mc.camera.setPos(3, 4, 5)
        require(mc.events.pollBlockHits() == [], "empty block hits")
        mc.events.clearAll()

        mc.saveCheckpoint()
        mc.setBlock(3, 3, 3, block.DIRT.id)
        mc.restoreCheckpoint()
        mc.postToChat("commas,stay,together")
        mc.postToChat("__mcpi_python_done__")

        code = host.wait(timeout=10)
        stderr = host.stderr.read() if host.stderr is not None else ""
        require(code == 0, f"host exit {code}: {stderr}")
        return 0
    finally:
        if host.poll() is None:
            host.kill()
            host.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
