package pi.event;

import pi.Vec;

public final class BlockAddedEvent extends BlockEvent {
    public BlockAddedEvent(Vec pos) { super(pos); }
}
