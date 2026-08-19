#pragma once

#include "game/GameState.hpp"

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>

namespace mcpi::client {

struct ClientOptions {
    std::filesystem::path world_path = "world.mcpiworld";
    std::optional<std::uint32_t> seed;
};

class ClientApp {
public:
    ClientApp(game::GameState& game, std::recursive_mutex& game_mutex, ClientOptions options);
    [[nodiscard]] int run();

private:
    game::GameState& game_;
    std::recursive_mutex& game_mutex_;
    ClientOptions options_;
};

} // namespace mcpi::client
