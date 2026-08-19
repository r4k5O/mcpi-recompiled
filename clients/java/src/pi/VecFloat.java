package pi;

import java.util.Objects;

/** Floating-point MCPI coordinate vector. */
public final class VecFloat {
    public static final VecFloat ZERO = new VecFloat(0.0f, 0.0f, 0.0f);

    public final float x;
    public final float y;
    public final float z;

    private VecFloat(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public static VecFloat xyz(float x, float y, float z) {
        return new VecFloat(x, y, z);
    }

    public VecFloat add(VecFloat other) { return xyz(x + other.x, y + other.y, z + other.z); }
    public VecFloat sub(VecFloat other) { return xyz(x - other.x, y - other.y, z - other.z); }
    public VecFloat mul(float scale) { return xyz(x * scale, y * scale, z * scale); }
    public VecFloat neg() { return mul(-1.0f); }
    public float dot(VecFloat other) { return x * other.x + y * other.y + z * other.z; }
    public float lengthSq() { return dot(this); }
    public float length() { return (float)Math.sqrt(lengthSq()); }
    public VecFloat normalized() {
        float length = length();
        return length == 0.0f ? ZERO : mul(1.0f / length);
    }

    static VecFloat decode(String value) {
        String[] parts = value.split(",");
        if (parts.length != 3) throw new IllegalArgumentException("invalid VecFloat: " + value);
        return xyz(Float.parseFloat(parts[0]), Float.parseFloat(parts[1]), Float.parseFloat(parts[2]));
    }

    @Override public String toString() { return x + "," + y + "," + z; }
    @Override public boolean equals(Object other) {
        return other instanceof VecFloat v && Float.compare(x, v.x) == 0 &&
               Float.compare(y, v.y) == 0 && Float.compare(z, v.z) == 0;
    }
    @Override public int hashCode() { return Objects.hash(x, y, z); }
}
