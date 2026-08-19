package pi;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.Closeable;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

final class Connection implements Closeable {
    private final Socket socket;
    private final BufferedReader input;
    private final BufferedWriter output;
    private boolean autoFlush = true;

    Connection(String host, int port) {
        try {
            socket = new Socket(host, port);
            socket.setTcpNoDelay(true);
            socket.setKeepAlive(true);
            input = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.US_ASCII));
            output = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.US_ASCII));
        } catch (IOException exception) {
            throw new ConnectionException("Could not connect to Minecraft Pi API at " + host + ":" + port, exception);
        }
    }

    synchronized void send(String command, Object... arguments) {
        try {
            output.write(command);
            output.write('(');
            for (int i = 0; i < arguments.length; ++i) {
                if (i != 0) output.write(',');
                output.write(String.valueOf(arguments[i]));
            }
            output.write(")\n");
            if (autoFlush) output.flush();
        } catch (IOException exception) {
            throw new ConnectionException("MCPI send failed", exception);
        }
    }

    synchronized String request(String command, Object... arguments) {
        if (!autoFlush) {
            throw new IllegalStateException("request methods require autoFlush(true)");
        }
        send(command, arguments);
        try {
            String response = input.readLine();
            if (response == null) throw new ConnectionException("MCPI connection closed while waiting for a response");
            return response;
        } catch (IOException exception) {
            throw new ConnectionException("MCPI receive failed", exception);
        }
    }

    synchronized void autoFlush(boolean enabled) {
        autoFlush = enabled;
        if (enabled) flush();
    }

    synchronized void flush() {
        try {
            output.flush();
        } catch (IOException exception) {
            throw new ConnectionException("MCPI flush failed", exception);
        }
    }

    @Override public synchronized void close() {
        try { output.close(); } catch (IOException ignored) {}
        try { input.close(); } catch (IOException ignored) {}
        try { socket.close(); } catch (IOException ignored) {}
    }

    static final class ConnectionException extends RuntimeException {
        ConnectionException(String message) { super(message); }
        ConnectionException(String message, Throwable cause) { super(message, cause); }
    }
}
