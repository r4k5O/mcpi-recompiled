package pi;

import java.util.ArrayList;
import java.util.List;
import pi.event.BlockHitEvent;

/** Package-level decoder for event transcript responses. */
final class EventFactory {
    private EventFactory() {}

    static List<BlockHitEvent> createBlockHitEvents(String eventList) {
        List<BlockHitEvent> events = new ArrayList<>();
        if (eventList == null || eventList.isEmpty()) return events;
        for (String event : eventList.split("\\|")) {
            String[] fields = event.split(",");
            if (fields.length != 5) continue;
            Vec pos = Vec.xyz(Integer.parseInt(fields[0]), Integer.parseInt(fields[1]), Integer.parseInt(fields[2]));
            events.add(new BlockHitEvent(pos, Integer.parseInt(fields[3]), Integer.parseInt(fields[4])));
        }
        return events;
    }
}
