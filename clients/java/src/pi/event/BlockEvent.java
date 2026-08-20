package pi.event;

import pi.Vec;

/** Base type for block-related API events. */
public class BlockEvent {
    public final Vec pos;
    public BlockEvent(Vec pos) { this.pos = pos; }
}
