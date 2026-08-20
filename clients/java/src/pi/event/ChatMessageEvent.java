package pi.event;

/** Chat event compatibility value. */
public final class ChatMessageEvent extends PlayerEvent {
    public final String message;
    public ChatMessageEvent(int entityId, String message) {
        super(entityId);
        this.message = message;
    }
}
