package pi.event;

import pi.Vec;

/** Block-hit event returned by events.block.hits(). */
public final class BlockHitEvent {
    public final Vec pos;
    public final int face;
    public final int entityId;

    public BlockHitEvent(Vec pos, int face, int entityId) {
        this.pos = pos;
        this.face = face;
        this.entityId = entityId;
    }
}
