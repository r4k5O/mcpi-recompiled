#include "world/RandomLevelSource.hpp"

#include <algorithm>

namespace mcpi::world {

RandomLevelSource::RandomLevelSource(std::uint32_t seed) noexcept
    : seed_(seed) {}

std::uint32_t RandomLevelSource::observed_chunk_coordinate_mix(
    int chunk_x,
    int chunk_z) noexcept {
    // ARM 32-bit integer arithmetic preserves the low 32 bits. Converting the
    // coordinates to uint32_t gives the same modulo-2^32 behavior for negative
    // chunk coordinates without relying on signed overflow.
    return 0x07ebe2d5U * static_cast<std::uint32_t>(chunk_z) +
           0x14609048U * static_cast<std::uint32_t>(chunk_x);
}

std::uint32_t RandomLevelSource::phase1_hash(std::uint32_t seed, int x, int z) noexcept {
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

int RandomLevelSource::phase1_surface_height(
    std::uint32_t seed,
    int world_x,
    int world_z) noexcept {
    world_x = std::clamp(world_x, 0, 255);
    world_z = std::clamp(world_z, 0, 255);

    const int grid_x = world_x >> 4;
    const int grid_z = world_z >> 4;
    const int local_x = world_x & 15;
    const int local_z = world_z & 15;

    const auto sample = [&](int sample_x, int sample_z) {
        return static_cast<int>(phase1_hash(seed, sample_x, sample_z) % 16U);
    };

    const int h00 = sample(grid_x, grid_z);
    const int h10 = sample(grid_x + 1, grid_z);
    const int h01 = sample(grid_x, grid_z + 1);
    const int h11 = sample(grid_x + 1, grid_z + 1);

    const int top = h00 * (16 - local_x) + h10 * local_x;
    const int bottom = h01 * (16 - local_x) + h11 * local_x;
    const int blended = (top * (16 - local_z) + bottom * local_z + 128) / 256;

    return 58 + blended;
}

Chunk RandomLevelSource::generate_chunk(int chunk_x, int chunk_z) const {
    Chunk chunk({chunk_x, chunk_z});

    for (int local_x = 0; local_x < Chunk::width; ++local_x) {
        for (int local_z = 0; local_z < Chunk::depth; ++local_z) {
            const int world_x = chunk_x * Chunk::width + local_x;
            const int world_z = chunk_z * Chunk::depth + local_z;
            const int surface = phase1_surface_height(seed_, world_x, world_z);

            chunk.set_block({local_x, 0, local_z}, {7, 0}); // bedrock

            for (int y = 1; y < surface - 3; ++y) {
                chunk.set_block({local_x, y, local_z}, {1, 0}); // stone
            }
            for (int y = std::max(1, surface - 3); y < surface; ++y) {
                chunk.set_block({local_x, y, local_z}, {3, 0}); // dirt
            }
            chunk.set_block({local_x, surface, local_z}, {2, 0}); // grass
        }
    }

    return chunk;
}

} // namespace mcpi::world
