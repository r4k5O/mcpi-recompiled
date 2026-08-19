#include "world/WorldGenerator.hpp"

#include <algorithm>

namespace mcpi::world {

std::uint32_t WorldGenerator::hash(std::uint32_t seed, int x, int z) noexcept {
    std::uint32_t value = seed;
    value ^= static_cast<std::uint32_t>(x) * 0x9e3779b9U;
    value ^= static_cast<std::uint32_t>(z) * 0x85ebca6bU;
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    value *= 0x846ca68bU;
    value ^= value >> 16U;
    return value;
}

int WorldGenerator::surface_height(std::uint32_t seed, int x, int z) noexcept {
    x = std::clamp(x, 0, 255);
    z = std::clamp(z, 0, 255);

    const int grid_x = x >> 4;
    const int grid_z = z >> 4;
    const int local_x = x & 15;
    const int local_z = z & 15;

    const auto sample = [&](int sx, int sz) {
        return static_cast<int>(hash(seed, sx, sz) % 16U);
    };

    const int h00 = sample(grid_x, grid_z);
    const int h10 = sample(grid_x + 1, grid_z);
    const int h01 = sample(grid_x, grid_z + 1);
    const int h11 = sample(grid_x + 1, grid_z + 1);

    const int top = h00 * (16 - local_x) + h10 * local_x;
    const int bottom = h01 * (16 - local_x) + h11 * local_x;
    const int blended = (top * (16 - local_z) + bottom * local_z + 128) / 256;

    // Phase-1 compatibility terrain profile. This finite profile is designed
    // to be deterministic and replaceable once RandomLevelSource parity is
    // fully reconstructed from the original binary.
    return 58 + blended;
}

void WorldGenerator::generate(World& world, std::uint32_t seed) {
    world.clear();

    for (int x = 0; x <= 255; ++x) {
        for (int z = 0; z <= 255; ++z) {
            const int surface = surface_height(seed, x, z);
            world.set_block({x, 0, z}, {7, 0}); // bedrock

            for (int y = 1; y < surface - 3; ++y) {
                world.set_block({x, y, z}, {1, 0}); // stone
            }
            for (int y = std::max(1, surface - 3); y < surface; ++y) {
                world.set_block({x, y, z}, {3, 0}); // dirt
            }
            world.set_block({x, surface, z}, {2, 0}); // grass
        }
    }
}

} // namespace mcpi::world
