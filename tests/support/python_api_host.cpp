#include "api/ApiDispatcher.hpp"
#include "api/ApiServer.hpp"
#include "game/GameState.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    mcpi::game::GameState game;
    game.set_player_position({1.5, 2.0, 3.25});

    mcpi::api::ApiDispatcher dispatcher(game);
    mcpi::api::ApiServer server(0);
    std::atomic<bool> chat_received{false};

    if (!server.start([&](const mcpi::api::Command& command) {
            auto response = dispatcher.dispatch(command);
            if (command.name == "chat.post") {
                chat_received.store(true);
            }
            return response;
        })) {
        std::cerr << "Failed to start Python API smoke-test host.\n";
        return 1;
    }

    std::cout << server.port() << std::endl;

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!chat_received.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    const bool completed_protocol_sequence = chat_received.load();
    server.stop();

    if (!completed_protocol_sequence) {
        std::cerr << "Python client did not complete the expected MCPI command sequence.\n";
        return 2;
    }

    const auto block = game.world().block_at({1, 2, 3});
    if (block.id != 57 || block.data != 0) {
        std::cerr << "Python setBlock did not persist in the reconstructed world.\n";
        return 3;
    }

    if (game.chat_messages().size() != 1 ||
        game.chat_messages().front() != "Hello from Python") {
        std::cerr << "Python postToChat did not persist in GameState.\n";
        return 4;
    }

    return 0;
}
