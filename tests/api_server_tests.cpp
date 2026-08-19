#include "api/ApiServer.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using TestSocket = SOCKET;
constexpr TestSocket kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using TestSocket = int;
constexpr TestSocket kInvalidSocket = -1;
#endif

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void close_socket(TestSocket socket) {
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

TestSocket connect_loopback(std::uint16_t port) {
    TestSocket socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == kInvalidSocket) {
        return kInvalidSocket;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (::connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        close_socket(socket);
        return kInvalidSocket;
    }

    return socket;
}

bool send_all(TestSocket socket, const std::string& value) {
    std::size_t sent = 0;
    while (sent < value.size()) {
#ifdef _WIN32
        const int result = ::send(socket, value.data() + sent,
                                  static_cast<int>(value.size() - sent), 0);
#else
        const auto result = ::send(socket, value.data() + sent, value.size() - sent, 0);
#endif
        if (result <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

std::string receive_line(TestSocket socket) {
    std::string result;
    char byte = '\0';
    while (true) {
#ifdef _WIN32
        const int received = ::recv(socket, &byte, 1, 0);
#else
        const auto received = ::recv(socket, &byte, 1, 0);
#endif
        if (received <= 0 || byte == '\n') {
            break;
        }
        if (byte != '\r') {
            result.push_back(byte);
        }
    }
    return result;
}

} // namespace

int main() {
#ifdef _WIN32
    WSADATA data{};
    expect(WSAStartup(MAKEWORD(2, 2), &data) == 0, "Winsock should initialize for the test client");
#endif

    mcpi::api::ApiServer server(0);
    expect(server.start([](const mcpi::api::Command& command) -> std::optional<std::string> {
        if (command.name == "player.getPos") {
            return "1.5,2.0,3.25";
        }
        return std::nullopt;
    }), "API server should start");

    expect(server.port() != 0, "port 0 should select an available TCP port");

    const TestSocket client = connect_loopback(server.port());
    expect(client != kInvalidSocket, "client should connect to the API server over loopback");
    expect(send_all(client, "player.getPos()\n"), "client should send a complete MCPI command line");
    expect(receive_line(client) == "1.5,2.0,3.25", "server should return the handler response followed by a newline");

    close_socket(client);
    server.stop();
    expect(!server.running(), "server should report stopped after stop()");

#ifdef _WIN32
    WSACleanup();
#endif

    std::cout << "TCP API server integration test passed.\n";
    return 0;
}
