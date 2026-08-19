#include "api/ApiDispatcher.hpp"
#include "api/ApiServer.hpp"
#include "game/GameState.hpp"

#ifdef MCPI_BUILD_CLIENT
#include "client/ClientApp.hpp"
#endif

#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
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

bool parse_seed(std::string_view text, std::uint32_t& seed) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, seed);
    return result.ec == std::errc{} && result.ptr == end;
}

void print_usage(const char* executable) {
    std::cout
        << "Usage: " << executable << " [options]\n"
        << "  --port <0-65535>     MCPI API port (default: 4711)\n"
        << "  --headless           Run API/world runtime without a window\n"
        << "  --world <path>       World save path (default: world.mcpiworld)\n"
        << "  --seed <0-4294967295> Seed used when creating a new world\n"
        << "  --help, -h           Show this help\n";
}

} // namespace

int main(int argc, char** argv) {
    std::uint16_t port = 4711;
#ifdef MCPI_BUILD_CLIENT
    bool headless = false;
#else
    bool headless = true;
#endif
    std::filesystem::path world_path = "world.mcpiworld";
    std::optional<std::uint32_t> seed;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);

        if (argument == "--help" || argument == "-h") {
            print_usage(argv[0]);
            return 0;
        }

        if (argument == "--headless") {
            headless = true;
            continue;
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

        if (argument == "--world") {
            if (index + 1 >= argc) {
                std::cerr << "Missing --world path.\n";
                return 2;
            }
            world_path = argv[++index];
            continue;
        }

        if (argument == "--seed") {
            std::uint32_t parsed_seed = 0;
            if (index + 1 >= argc || !parse_seed(argv[index + 1], parsed_seed)) {
                std::cerr << "Invalid --seed value. Expected an unsigned 32-bit integer.\n";
                return 2;
            }
            seed = parsed_seed;
            ++index;
            continue;
        }

        std::cerr << "Unknown argument: " << argument << '\n';
        print_usage(argv[0]);
        return 2;
    }

    mcpi::game::GameState game;
    std::mutex game_mutex;

    if (headless) {
        if (std::filesystem::exists(world_path)) {
            if (!game.load(world_path)) {
                std::cerr << "Could not load world: " << world_path.string() << '\n';
                return 3;
            }
        } else if (seed.has_value()) {
            game.new_world(*seed);
        }
    }

    mcpi::api::ApiDispatcher dispatcher(game);
    mcpi::api::ApiServer server(port);

    if (!server.start([&dispatcher, &game_mutex](const mcpi::api::Command& command) {
            std::scoped_lock lock(game_mutex);
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

#ifdef MCPI_BUILD_CLIENT
    if (!headless) {
        mcpi::client::ClientApp app(
            game,
            game_mutex,
            mcpi::client::ClientOptions{world_path, seed});
        const int result = app.run();
        server.stop();
        return result;
    }
#endif

    while (keep_running != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    {
        std::scoped_lock lock(game_mutex);
        if (game.generated_world()) {
            game.save(world_path);
        }
    }

    server.stop();
    return 0;
}
