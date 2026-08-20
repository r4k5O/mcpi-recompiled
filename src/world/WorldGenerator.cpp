#include "world/WorldGenerator.hpp"

#include "world/RandomLevelSource.hpp"

namespace mcpi::world {

int WorldGenerator::surface_height(std::uint32_t seed, int x, int z) noexcept {
    return RandomLevelSource::phase1_surface_height(seed, x, z);
}

void WorldGenerator::generate(World& world, std::uint32_t seed) {
    world.clear();

    const RandomLevelSource source(seed);
    constexpr int chunks_per_axis = 256 / Chunk::width;

    for (int chunk_x = 0; chunk_x < chunks_per_axis; ++chunk_x) {
        for (int chunk_z = 0; chunk_z < chunks_per_axis; ++chunk_z) {
            const Chunk chunk = source.generate_chunk(chunk_x, chunk_z);

            for (int local_x = 0; local_x < Chunk::width; ++local_x) {
                for (int local_z = 0; local_z < Chunk::depth; ++local_z) {
                    const int column_height = chunk.height_at(local_x, local_z);
                    for (int y = 0; y < column_height; ++y) {
                        const BlockState block = chunk.block_at({local_x, y, local_z});
                        if (block.id == 0) {
                            continue;
                        }

                        world.set_block({
                            chunk_x * Chunk::width + local_x,
                            y,
                            chunk_z * Chunk::depth + local_z,
                        }, block);
                    }
                }
            }
        }
    }
}

} // namespace mcpi::world
