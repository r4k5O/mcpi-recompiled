package pi;

import java.util.Objects;

/** Integer MCPI coordinate vector. */
public final class Vec {
    public static final Vec ZERO = new Vec(0, 0, 0);
    public static final int MIN_Y = -128;
    public static final int MAX_Y = 127;

    public final int x;
    public final int y;
    public final int z;

    private Vec(int x, int y, int z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public static Vec xyz(int x, int y, int z) {
        return new Vec(x, y, z);
    }

    public Vec add(Vec other) { return xyz(x + other.x, y + other.y, z + other.z); }
    public Vec add(int dx, int dy, int dz) { return xyz(x + dx, y + dy, z + dz); }
    public Vec sub(Vec other) { return xyz(x - other.x, y - other.y, z - other.z); }
    public Vec mul(int scale) { return xyz(x * scale, y * scale, z * scale); }
    public Vec neg() { return xyz(-x, -y, -z); }
    public int dot(Vec other) { return x * other.x + y * other.y + z * other.z; }

    static Vec decode(String value) {
        String[] parts = value.split(",");
        if (parts.length != 3) throw new IllegalArgumentException("invalid Vec: " + value);
        return xyz(Integer.parseInt(parts[0]), Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
    }

    @Override public String toString() { return x + "," + y + "," + z; }
    @Override public boolean equals(Object other) {
        return other instanceof Vec v && x == v.x && y == v.y && z == v.z;
    }
    @Override public int hashCode() { return Objects.hash(x, y, z); }
}
