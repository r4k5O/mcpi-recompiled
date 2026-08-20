#pragma once

#include "world/World.hpp"

#include <cstdint>

namespace mcpi::world {

class LightEngine {
public:
    // Rebuild both packed light layers for all chunks currently present in the
    // world. This correctness-first path is intentionally deterministic; an
    // incremental queue can replace it later without changing the contract.
    void rebuild(World& world) const;

    // Correctness-first block-change hook. It deliberately rebuilds the known
    // chunk set so source removal cannot leave stale light behind.
    void on_block_changed(
        World& world,
        const BlockPos& position,
        const BlockState& before,
        const BlockState& after) const;

    [[nodiscard]] static std::uint8_t emission_for(const BlockState& block) noexcept;
    [[nodiscard]] static std::uint8_t opacity_for(const BlockState& block) noexcept;
};

} // namespace mcpi::world
