package pi;

import pi.event.BlockAddedEvent;
import pi.event.BlockEvent;
import pi.event.BlockHitEvent;
import pi.event.BlockRemovedEvent;
import pi.event.ChatMessageEvent;
import pi.event.EntityEvent;
import pi.event.PlayerConnectEvent;
import pi.event.PlayerEvent;
import pi.tool.Csg;
import pi.tool.Text;
import pi.tool.Tools;
import pi.tool.Turtle;

/** Compile-only contract for the class surface shipped with Minecraft Pi. */
public final class OriginalApiSurfaceSmoke {
    private OriginalApiSurfaceSmoke() {}

    static void coreSurface(Minecraft minecraft) {
        Block block = Block.STONE.withData(1);
        Vec tile = Vec.xyz(1, 2, 3);
        VecFloat exact = VecFloat.xyz(1.25f, 2.5f, 3.75f);
        minecraft.setBlock(tile, block);
        minecraft.player.setPosition(tile);
        minecraft.player.setExactPosition(exact);
        minecraft.camera.setNormal();
        minecraft.camera.setThirdPerson();
        minecraft.camera.setFixed();
        minecraft.events.pollBlockHits();
        minecraft.entities.getPosition(0);
        minecraft.postToChat("surface");
    }

    static void originalClassSurface(Turtle turtle) {
        Class<?>[] classes = {
            Color.class, Item.class, EventFactory.class, Log.class,
            BlockEvent.class, BlockAddedEvent.class, BlockRemovedEvent.class, BlockHitEvent.class,
            ChatMessageEvent.class, EntityEvent.class, PlayerEvent.class, PlayerConnectEvent.class,
            Csg.class, Text.class, Tools.class, Turtle.class,
        };
        if (classes.length != 16 || Color.WHITE.ordinal() != 0 || Color.BLACK.ordinal() != 15 || Item.CAMERA == null) {
            throw new AssertionError("original Java surface constants changed");
        }
        turtle.setHome(Vec.ZERO).home().on().off().block(Block.WOOD_PLANKS).jump(1, 2, 3)
              .left().right().around().forward(1).back(1).up(1).down(1);
    }
}
