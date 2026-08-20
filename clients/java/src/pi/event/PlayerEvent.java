package pi.event;

/** Base event for player-associated events. */
public class PlayerEvent extends EntityEvent {
    public PlayerEvent(int entityId) { super(entityId); }
}
