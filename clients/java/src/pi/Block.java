package pi;

import java.util.Objects;

/** MCPI block id plus the four-bit block-data value. */
public final class Block {
    public final int id;
    public final int data;

    private Block(int id, int data) {
        this.id = id;
        this.data = data & 0x0f;
    }

    public static Block id(int id) { return new Block(id, 0); }
    public Block withData(int value) { return new Block(id, value); }

    static Block decode(String value) {
        return id(Integer.parseInt(value.trim()));
    }

    static Block decodeWithData(String value) {
        String[] parts = value.split(",");
        if (parts.length != 2) throw new IllegalArgumentException("invalid block: " + value);
        return new Block(Integer.parseInt(parts[0]), Integer.parseInt(parts[1]));
    }

    @Override public String toString() { return data == 0 ? Integer.toString(id) : id + "," + data; }
    @Override public boolean equals(Object other) {
        return other instanceof Block b && id == b.id && data == b.data;
    }
    @Override public int hashCode() { return Objects.hash(id, data); }

    public static final Block AIR = id(0);
    public static final Block STONE = id(1);
    public static final Block GRASS = id(2);
    public static final Block DIRT = id(3);
    public static final Block COBBLESTONE = id(4);
    public static final Block WOOD_PLANKS = id(5);
    public static final Block BEDROCK = id(7);
    public static final Block SAND = id(12);
    public static final Block GRAVEL = id(13);
    public static final Block WOOD = id(17);
    public static final Block LEAVES = id(18);
    public static final Block GLASS = id(20);
    public static final Block WOOL = id(35);
    public static final Block GOLD_BLOCK = id(41);
    public static final Block IRON_BLOCK = id(42);
    public static final Block BRICK_BLOCK = id(45);
    public static final Block TNT = id(46);
    public static final Block OBSIDIAN = id(49);
    public static final Block DIAMOND_ORE = id(56);
    public static final Block DIAMOND_BLOCK = id(57);
    public static final Block CRAFTING_TABLE = id(58);
    public static final Block GLOWSTONE_BLOCK = id(89);
}
