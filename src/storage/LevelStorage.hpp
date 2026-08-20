#pragma once

#include <filesystem>

namespace mcpi::game {
class GameState;
}

namespace mcpi::storage {

class LevelStorage {
public:
    virtual ~LevelStorage() = default;

    virtual bool load(
        game::GameState& state,
        const std::filesystem::path& path) = 0;

    virtual bool save(
        const game::GameState& state,
        const std::filesystem::path& path) = 0;
};

} // namespace mcpi::storage
