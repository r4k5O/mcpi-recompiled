#pragma once

#include "storage/LevelStorage.hpp"

#include <cstddef>
#include <cstdint>

namespace mcpi::storage {

class PiLevelStorage final : public LevelStorage {
public:
    static constexpr std::uint32_t file_version = 3U;
    static constexpr std::size_t max_level_dat_payload = 16U * 1024U * 1024U;

    bool load(
        game::GameState& state,
        const std::filesystem::path& path) override;

    bool save(
        const game::GameState& state,
        const std::filesystem::path& path) override;
};

} // namespace mcpi::storage
