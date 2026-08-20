package pi.tool;

import pi.Block;
import pi.Minecraft;
import pi.Vec;

/** Minimal independent text helper preserving the original tool namespace. */
public final class Text {
    private Text() {}

    public static void draw(Minecraft minecraft, String text, Vec origin, Block block) {
        if (minecraft == null || text == null || origin == null || block == null) return;
        for (int i = 0; i < text.length(); ++i) {
            if (!Character.isWhitespace(text.charAt(i))) {
                minecraft.setBlock(origin.add(i, 0, 0), block);
            }
        }
    }
}
