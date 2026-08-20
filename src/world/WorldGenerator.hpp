#pragma once

#include "world/World.hpp"

#include <cstdint>

namespace mcpi::world {

class WorldGenerator {
public:
    static void generate(World& world, std::uint32_t seed);

    // Phase-1 compatibility entry point retained for callers/tests while the
    // real RandomLevelSource algorithm is reconstructed from reference data.
    [[nodiscard]] static int surface_height(std::uint32_t seed, int x, int z) noexcept;
};

} // namespace mcpi::world
