#pragma once

#include "world/World.hpp"

#include <cstdint>

namespace mcpi::world {

class WorldGenerator {
public:
    static void generate(World& world, std::uint32_t seed);
    [[nodiscard]] static int surface_height(std::uint32_t seed, int x, int z) noexcept;

private:
    [[nodiscard]] static std::uint32_t hash(std::uint32_t seed, int x, int z) noexcept;
};

} // namespace mcpi::world
