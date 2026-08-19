#include "api/ApiDispatcher.hpp"
#include "api/ApiServer.hpp"
#include "game/GameApi.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

namespace {

class SmokeGameApi final : public mcpi::game::GameApi {
public:
    mcpi::game::Vec3 player_position() const override {
        position_requested.store(true);
        return {1.5, 2.0, 3.25};
    }

    void set_player_position(const mcpi::game::Vec3&) override {}

    int block_type(int, int, int) const override {
        return 57;
    }

    void set_block(int x, int y, int z, int block_type, int block_data) override {
        if (x == 1 && y == 2 && z == 3 && block_type == 57 && block_data == 0) {
            block_set.store(true);
        }
    }

    void post_chat(const std::string& message) override {
        if (message == "Hello from Python") {
            chat_posted.store(true);
        }
    }

    [[nodiscard]] bool complete() const {
        return position_requested.load() && block_set.load() && chat_posted.load();
    }

private:
    mutable std::atomic<bool> position_requested{false};
    std::atomic<bool> block_set{false};
    std::atomic<bool> chat_posted{false};
};

} // namespace

int main() {
    SmokeGameApi game;
    mcpi::api::ApiDispatcher dispatcher(game);
    mcpi::api::ApiServer server(0);

    if (!server.start([&dispatcher](const mcpi::api::Command& command) {
            return dispatcher.dispatch(command);
        })) {
        std::cerr << "Failed to start Python API smoke-test host.\n";
        return 1;
    }

    std::cout << server.port() << std::endl;

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!game.complete() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    const bool complete = game.complete();
    server.stop();

    if (!complete) {
        std::cerr << "Python client did not complete the expected MCPI command sequence.\n";
        return 2;
    }

    return 0;
}
