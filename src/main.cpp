#include "api/ApiDispatcher.hpp"
#include "api/ApiServer.hpp"
#include "game/GameState.hpp"

#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <thread>

namespace {

volatile std::sig_atomic_t keep_running = 1;

void handle_signal(int) {
    keep_running = 0;
}

bool parse_port(std::string_view text, std::uint16_t& port) {
    if (text.empty()) {
        return false;
    }

    unsigned int value = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end || value > 65535U) {
        return false;
    }

    port = static_cast<std::uint16_t>(value);
    return true;
}

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable << " [--port <0-65535>]\n";
}

} // namespace

int main(int argc, char** argv) {
    std::uint16_t port = 4711;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);

        if (argument == "--help" || argument == "-h") {
            print_usage(argv[0]);
            return 0;
        }

        if (argument == "--port") {
            if (index + 1 >= argc || !parse_port(argv[index + 1], port)) {
                std::cerr << "Invalid --port value. Expected an integer from 0 to 65535.\n";
                print_usage(argv[0]);
                return 2;
            }
            ++index;
            continue;
        }

        std::cerr << "Unknown argument: " << argument << '\n';
        print_usage(argv[0]);
        return 2;
    }

    mcpi::game::GameState game;
    mcpi::api::ApiDispatcher dispatcher(game);
    mcpi::api::ApiServer server(port);

    if (!server.start([&dispatcher](const mcpi::api::Command& command) {
            return dispatcher.dispatch(command);
        })) {
        std::cerr << "Failed to start MCPI API server on port " << port << ".\n";
        return 1;
    }

    std::signal(SIGINT, handle_signal);
#ifdef SIGTERM
    std::signal(SIGTERM, handle_signal);
#endif

    std::cout << "MCPI API listening on port " << server.port() << std::endl;

    while (keep_running != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server.stop();
    return 0;
}
