package pi;

import java.util.ArrayList;
import java.util.List;
import pi.event.BlockHitEvent;

/** Java client for the Minecraft Pi programming protocol. */
public final class Minecraft implements AutoCloseable {
    public static final int DEFAULT_PORT = 4711;

    private final Connection connection;
    public final Player player = new Player();
    public final Camera camera = new Camera();
    public final Events events = new Events();
    public final Entities entities = new Entities();

    private Minecraft(Connection connection) {
        this.connection = connection;
    }

    public static Minecraft connect() { return connect("127.0.0.1", DEFAULT_PORT); }
    public static Minecraft connect(String host) { return connect(host, DEFAULT_PORT); }
    public static Minecraft connect(String host, int port) { return new Minecraft(new Connection(host, port)); }

    public Block getBlock(Vec position) {
        return Block.decode(connection.request("world.getBlock", position));
    }

    public Block getBlockWithData(Vec position) {
        return Block.decodeWithData(connection.request("world.getBlockWithData", position));
    }

    public void setBlock(int x, int y, int z, Block block) { setBlock(Vec.xyz(x, y, z), block); }
    public void setBlock(Vec position, Block block) { connection.send("world.setBlock", position, block); }

    public void setBlocks(int x1, int y1, int z1, int x2, int y2, int z2, Block block) {
        setBlocks(Vec.xyz(x1, y1, z1), Vec.xyz(x2, y2, z2), block);
    }

    public void setBlocks(Vec begin, Vec end, Block block) {
        connection.send("world.setBlocks", begin, end, block);
    }

    public int getHeight(int x, int z) {
        return Integer.parseInt(connection.request("world.getHeight", x, z));
    }

    public int[] getPlayerEntityIds() {
        String value = connection.request("world.getPlayerIds");
        if (value.isBlank()) return new int[0];
        String[] parts = value.split("\\|");
        int[] ids = new int[parts.length];
        for (int i = 0; i < parts.length; ++i) ids[i] = Integer.parseInt(parts[i]);
        return ids;
    }

    public void setting(String key, boolean value) { connection.send("world.setting", key, value ? 1 : 0); }
    public void saveCheckpoint() { connection.send("world.checkpoint.save"); }
    public void restoreCheckpoint() { connection.send("world.checkpoint.restore"); }
    public void postToChat(String message) { connection.send("chat.post", message); }
    public void autoFlush(boolean enabled) { connection.autoFlush(enabled); }
    public void flush() { connection.flush(); }

    @Override public void close() { connection.close(); }

    public final class Player {
        public Vec getPosition() { return Vec.decode(connection.request("player.getTile")); }
        public void setPosition(Vec position) { connection.send("player.setTile", position); }
        public VecFloat getExactPosition() { return VecFloat.decode(connection.request("player.getPos")); }
        public void setExactPosition(VecFloat position) { connection.send("player.setPos", position); }
        public void setting(String key, boolean value) { connection.send("player.setting", key, value ? 1 : 0); }
    }

    public final class Camera {
        public void setNormal() { connection.send("camera.mode.setNormal"); }
        public void setNormal(int entityId) { connection.send("camera.mode.setNormal", entityId); }
        public void setThirdPerson() { connection.send("camera.mode.setFollow"); }
        public void setThirdPerson(int entityId) { connection.send("camera.mode.setFollow", entityId); }
        public void setFixed() { connection.send("camera.mode.setFixed"); }
        public void setPosition(VecFloat position) { connection.send("camera.setPos", position); }
    }

    public final class Events {
        public void clearAll() { connection.send("events.clear"); }

        public List<BlockHitEvent> pollBlockHits() {
            String encoded = connection.request("events.block.hits");
            List<BlockHitEvent> result = new ArrayList<>();
            if (encoded.isBlank()) return result;
            for (String entry : encoded.split("\\|")) {
                if (entry.isBlank()) continue;
                String[] parts = entry.split(",");
                if (parts.length != 5) throw new IllegalArgumentException("invalid block-hit event: " + entry);
                result.add(new BlockHitEvent(
                    Vec.xyz(Integer.parseInt(parts[0]), Integer.parseInt(parts[1]), Integer.parseInt(parts[2])),
                    Integer.parseInt(parts[3]),
                    Integer.parseInt(parts[4])));
            }
            return result;
        }
    }

    public final class Entities {
        public Vec getPosition(int entityId) {
            return Vec.decode(connection.request("entity.getTile", entityId));
        }
        public void setPosition(int entityId, Vec position) {
            connection.send("entity.setTile", entityId, position);
        }
        public VecFloat getExactPosition(int entityId) {
            return VecFloat.decode(connection.request("entity.getPos", entityId));
        }
        public void setExactPosition(int entityId, VecFloat position) {
            connection.send("entity.setPos", entityId, position);
        }
    }
}
