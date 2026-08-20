package pi;

import java.util.Objects;

/** Item identifier value used by the original Java surface. */
public final class Item {
    public final int id;

    Item(int id) { this.id = id; }
    static Item id(int id) { return new Item(id); }
    static Item decode(String value) { return id(Integer.parseInt(value.trim())); }

    @Override public String toString() { return Integer.toString(id); }
    @Override public boolean equals(Object other) { return other instanceof Item item && id == item.id; }
    @Override public int hashCode() { return Objects.hash(id); }

    public static final Item DIAMOND = id(264);
    public static final Item STICK = id(280);
    public static final Item PAPER = id(339);
    public static final Item BOOK = id(340);
    public static final Item COMPASS = id(345);
    public static final Item CLOCK = id(347);
    public static final Item CAMERA = id(456);
}
