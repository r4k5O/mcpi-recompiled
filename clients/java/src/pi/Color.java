package pi;

/** Wool-color data values used by the Pi API. */
public enum Color {
    WHITE, ORANGE, MAGENTA, LIGHT_BLUE,
    YELLOW, LIME, PINK, GRAY,
    LIGHT_GRAY, CYAN, PURPLE, BLUE,
    BROWN, GREEN, RED, BLACK;

    public int data() { return ordinal(); }
}
