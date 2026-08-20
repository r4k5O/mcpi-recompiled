package pi.tool;

import pi.Block;
import pi.Minecraft;
import pi.Vec;

/** Independent fluent turtle compatible with the Pi Java demo surface. */
public final class Turtle {
    private final Minecraft minecraft;
    private Vec home = Vec.ZERO;
    private Vec pos = Vec.ZERO;
    private Vec dir = Vec.xyz(1, 0, 0);
    private Block block = Block.WOOD_PLANKS;
    private boolean placing;

    public Turtle(Minecraft minecraft) { this.minecraft = minecraft; }
    public Turtle setHome(Vec value) { home = value; return this; }
    public Turtle home() { pos = Vec.ZERO; dir = Vec.xyz(1, 0, 0); return this; }
    public Turtle on() { placing = true; place(); return this; }
    public Turtle off() { placing = false; return this; }
    public Turtle block(Block value) { block = value; return this; }
    public Turtle jump(int dx, int dy, int dz) { pos = pos.add(dx, dy, dz); return this; }
    public Turtle left() { dir = Vec.xyz(dir.z, 0, -dir.x); return this; }
    public Turtle right() { dir = Vec.xyz(-dir.z, 0, dir.x); return this; }
    public Turtle around() { dir = Vec.xyz(-dir.x, 0, -dir.z); return this; }
    public Turtle forward(int steps) { return move(steps, dir); }
    public Turtle back(int steps) { return move(steps, dir.neg()); }
    public Turtle up(int steps) { return move(steps, Vec.xyz(0, 1, 0)); }
    public Turtle down(int steps) { return move(steps, Vec.xyz(0, -1, 0)); }

    private Turtle move(int steps, Vec delta) {
        int count = Math.max(0, steps);
        while (count-- > 0) { pos = pos.add(delta); place(); }
        return this;
    }

    private void place() {
        if (placing && minecraft != null) minecraft.setBlock(home.add(pos), block);
    }
}
