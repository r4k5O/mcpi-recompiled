#include "api/ApiDispatcher.hpp"
#include "api/ApiServer.hpp"
#include "game/GameState.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    mcpi::game::GameState game;
    game.world().clear();
    game.set_spawn_position({0, 0, 0});
    game.set_player_position({1.5, 2.0, 3.25});

    mcpi::api::ApiDispatcher dispatcher(game);
    mcpi::api::ApiServer server(0);
    std::atomic<bool> done{false};

    if (!server.start([&](const mcpi::api::Command& command) {
            const auto result = dispatcher.dispatch_result(command);
            if (command.name == "chat.post" && command.arguments.size() == 1U &&
                command.arguments.front() == "__mcpi_python_done__") {
                done.store(true);
            }
            return result;
        })) {
        return 1;
    }

    std::cout << server.port() << std::endl;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (!done.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    server.stop();
    return done.load() ? 0 : 2;
}
