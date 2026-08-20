#include "api/ApiServer.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using NativeSocket = SOCKET;
constexpr NativeSocket kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
using NativeSocket = int;
constexpr NativeSocket kInvalidSocket = -1;
#endif

namespace mcpi::api {
namespace {

void close_socket(NativeSocket socket) {
    if (socket == kInvalidSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

void configure_receive_timeout(NativeSocket socket) {
#ifdef _WIN32
    const DWORD timeout_ms = 200;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
#else
    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
}

bool receive_timed_out() {
#ifdef _WIN32
    const int error = WSAGetLastError();
    return error == WSAETIMEDOUT || error == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

bool send_all(NativeSocket socket, const std::string& payload) {
    std::size_t sent = 0;
    while (sent < payload.size()) {
#ifdef _WIN32
        const int result = ::send(socket, payload.data() + sent,
                                  static_cast<int>(payload.size() - sent), 0);
#else
#ifdef MSG_NOSIGNAL
        const auto result = ::send(socket, payload.data() + sent, payload.size() - sent, MSG_NOSIGNAL);
#else
        const auto result = ::send(socket, payload.data() + sent, payload.size() - sent, 0);
#endif
#endif
        if (result <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

} // namespace

struct ApiServer::Impl {
    explicit Impl(std::uint16_t requested_port)
        : requested_port(requested_port), bound_port(requested_port) {}

    void run(NativeSocket listening_socket) {
        while (is_running.load()) {
            const NativeSocket client = ::accept(listening_socket, nullptr, nullptr);
            if (client == kInvalidSocket) {
                if (!is_running.load()) {
                    break;
                }
                continue;
            }

            configure_receive_timeout(client);
            handle_client(client);
            close_socket(client);
        }
    }

    void handle_client(NativeSocket client) {
        std::string pending;
        char buffer[4096];

        while (is_running.load()) {
#ifdef _WIN32
            const int received = ::recv(client, buffer, static_cast<int>(sizeof(buffer)), 0);
#else
            const auto received = ::recv(client, buffer, sizeof(buffer), 0);
#endif
            if (received == 0) {
                return;
            }
            if (received < 0) {
                if (receive_timed_out()) {
                    continue;
                }
#ifndef _WIN32
                if (errno == EINTR) {
                    continue;
                }
#endif
                return;
            }

            pending.append(buffer, static_cast<std::size_t>(received));

            std::size_t newline = std::string::npos;
            while ((newline = pending.find('\n')) != std::string::npos) {
                std::string line = pending.substr(0, newline);
                pending.erase(0, newline + 1);

                const auto command = parse_command(line);
                if (!command || !handler) {
                    continue;
                }

                const auto response = handler(*command);
                if (response && !send_all(client, *response + "\n")) {
                    return;
                }
            }
        }
    }

    std::uint16_t requested_port = 4711;
    std::uint16_t bound_port = 4711;
    NativeSocket listener = kInvalidSocket;
    Handler handler;
    std::atomic<bool> is_running{false};
    std::thread worker;
#ifdef _WIN32
    bool winsock_started = false;
#endif
};

ApiServer::ApiServer(std::uint16_t port)
    : impl_(std::make_unique<Impl>(port)) {}

ApiServer::~ApiServer() {
    stop();
}

bool ApiServer::start(Handler handler) {
    if (impl_->is_running.load() || !handler) {
        return false;
    }

#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }
    impl_->winsock_started = true;
#endif

    const NativeSocket listener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == kInvalidSocket) {
#ifdef _WIN32
        WSACleanup();
        impl_->winsock_started = false;
#endif
        return false;
    }

    int reuse = 1;
#ifdef _WIN32
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));
#else
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(impl_->requested_port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        close_socket(listener);
#ifdef _WIN32
        WSACleanup();
        impl_->winsock_started = false;
#endif
        return false;
    }

    if (::listen(listener, 4) != 0) {
        close_socket(listener);
#ifdef _WIN32
        WSACleanup();
        impl_->winsock_started = false;
#endif
        return false;
    }

    if (impl_->requested_port == 0) {
        sockaddr_in bound{};
#ifdef _WIN32
        int length = sizeof(bound);
#else
        socklen_t length = sizeof(bound);
#endif
        if (::getsockname(listener, reinterpret_cast<sockaddr*>(&bound), &length) != 0) {
            close_socket(listener);
#ifdef _WIN32
            WSACleanup();
            impl_->winsock_started = false;
#endif
            return false;
        }
        impl_->bound_port = ntohs(bound.sin_port);
    } else {
        impl_->bound_port = impl_->requested_port;
    }

    impl_->listener = listener;
    impl_->handler = std::move(handler);
    impl_->is_running.store(true);
    impl_->worker = std::thread([impl = impl_.get(), listener] {
        impl->run(listener);
    });
    return true;
}

void ApiServer::stop() {
    if (!impl_->is_running.exchange(false)) {
        return;
    }

    close_socket(impl_->listener);

    if (impl_->worker.joinable()) {
        impl_->worker.join();
    }

    impl_->listener = kInvalidSocket;
    impl_->handler = {};

#ifdef _WIN32
    if (impl_->winsock_started) {
        WSACleanup();
        impl_->winsock_started = false;
    }
#endif
}

bool ApiServer::running() const noexcept {
    return impl_->is_running.load();
}

std::uint16_t ApiServer::port() const noexcept {
    return impl_->bound_port;
}

} // namespace mcpi::api
