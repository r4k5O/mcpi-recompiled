package pi;

/** Small package-local logger retained for source compatibility. */
final class Log {
    private Log() {}
    static void debug(String value) { }
    static void info(String value) { System.out.println(value); }
    static void error(String value) { System.err.println(value); }
}
