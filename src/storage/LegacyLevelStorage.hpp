#pragma once

#include "storage/LevelStorage.hpp"

namespace mcpi::storage {

class LegacyLevelStorage final : public LevelStorage {
public:
    bool load(
        game::GameState& state,
        const std::filesystem::path& path) override;

    bool save(
        const game::GameState& state,
        const std::filesystem::path& path) override;
};

} // namespace mcpi::storage
