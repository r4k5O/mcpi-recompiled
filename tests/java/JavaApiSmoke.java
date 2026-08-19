import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import pi.Block;
import pi.Minecraft;
import pi.Vec;
import pi.VecFloat;

public final class JavaApiSmoke {
    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 1) throw new IllegalArgumentException("expected mcpi-recompiled executable path");

        Process process = new ProcessBuilder(args[0], "--headless", "--port", "0")
            .redirectErrorStream(true)
            .start();

        try (BufferedReader output = new BufferedReader(
                 new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
            String firstLine = output.readLine();
            require(firstLine != null && firstLine.startsWith("MCPI API listening on port "),
                    "main executable did not publish API port: " + firstLine);
            int port = Integer.parseInt(firstLine.substring("MCPI API listening on port ".length()));

            try (Minecraft mc = Minecraft.connect("127.0.0.1", port)) {
                mc.player.setExactPosition(VecFloat.xyz(10.5f, 20.0f, -3.25f));
                VecFloat pos = mc.player.getExactPosition();
                require(Math.abs(pos.x - 10.5f) < 0.001f &&
                        Math.abs(pos.y - 20.0f) < 0.001f &&
                        Math.abs(pos.z + 3.25f) < 0.001f,
                        "Java player exact position did not round-trip: " + pos);

                mc.setBlock(Vec.xyz(4, 5, 6), Block.DIAMOND_BLOCK.withData(3));
                require(mc.getBlockWithData(Vec.xyz(4, 5, 6)).equals(Block.DIAMOND_BLOCK.withData(3)),
                        "Java block id/data did not round-trip");

                mc.setBlocks(-1, 1, -1, 1, 2, 1, Block.GOLD_BLOCK);
                require(mc.getBlock(Vec.xyz(-1, 1, -1)).equals(Block.GOLD_BLOCK),
                        "Java setBlocks start corner missing");
                require(mc.getBlock(Vec.xyz(1, 2, 1)).equals(Block.GOLD_BLOCK),
                        "Java setBlocks end corner missing");

                mc.setBlock(Vec.xyz(8, 5, 8), Block.STONE);
                require(mc.getHeight(8, 8) >= 6,
                        "Java getHeight did not observe placed block");

                mc.saveCheckpoint();
                mc.setBlock(Vec.xyz(4, 5, 6), Block.AIR);
                require(mc.getBlock(Vec.xyz(4, 5, 6)).equals(Block.AIR),
                        "checkpoint setup failed");
                mc.restoreCheckpoint();
                require(mc.getBlock(Vec.xyz(4, 5, 6)).equals(Block.DIAMOND_BLOCK),
                        "checkpoint restore did not restore world state");

                require(mc.getPlayerEntityIds().length == 1 && mc.getPlayerEntityIds()[0] == 0,
                        "local player should be exposed as entity 0 in Phase 1");

                mc.setting("world_immutable", false);
                mc.player.setting("autojump", true);
                mc.camera.setThirdPerson();
                mc.camera.setPosition(VecFloat.xyz(1.0f, 2.0f, 3.0f));
                mc.events.clearAll();
                require(mc.events.pollBlockHits().isEmpty(), "new game should have no queued block-hit events");
                mc.postToChat("Hello from Java Phase 1");
            }

            System.out.println("Java MCPI API smoke test passed.");
        } finally {
            process.destroy();
            if (!process.waitFor(5, java.util.concurrent.TimeUnit.SECONDS)) {
                process.destroyForcibly();
                process.waitFor();
            }
        }
    }
}
