package pi.tool;

/** General Java API helper methods. */
public final class Tools {
    private Tools() {}
    public static void sleep(long millis) {
        try { Thread.sleep(Math.max(0L, millis)); }
        catch (InterruptedException interrupted) { Thread.currentThread().interrupt(); }
    }
}
