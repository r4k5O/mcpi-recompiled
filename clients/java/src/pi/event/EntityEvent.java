package pi.event;

/** Base event carrying an MCPI entity id. */
public class EntityEvent {
    public final int entityId;
    public EntityEvent(int entityId) { this.entityId = entityId; }
}
