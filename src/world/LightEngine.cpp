#include "world/LightEngine.hpp"

#include <algorithm>
#include <array>
#include <queue>

namespace mcpi::world {
namespace {

constexpr std::array<BlockPos, 6> neighbor_offsets{{
    {1, 0, 0},
    {-1, 0, 0},
    {0, 1, 0},
    {0, -1, 0},
    {0, 0, 1},
    {0, 0, -1},
}};

BlockPos add(const BlockPos& position, const BlockPos& delta) noexcept {
    return {
        position.x + delta.x,
        position.y + delta.y,
        position.z + delta.z,
    };
}

} // namespace

std::uint8_t LightEngine::emission_for(const BlockState& block) noexcept {
    switch (block.id) {
    case 10: // flowing lava
    case 11: // still lava
    case 51: // fire
    case 89: // glowstone
    case 91: // jack o'lantern
        return 15U;
    case 50: // torch
        return 14U;
    default:
        return 0U;
    }
}

std::uint8_t LightEngine::opacity_for(const BlockState& block) noexcept {
    switch (block.id) {
    case 0:  // air
    case 6:  // sapling
    case 20: // glass
    case 37: // yellow flower
    case 38: // cyan flower
    case 39: // brown mushroom
    case 40: // red mushroom
    case 50: // torch
    case 51: // fire
        return 0U;
    case 8:  // flowing water
    case 9:  // still water
        return 2U;
    case 18: // leaves
        return 1U;
    default:
        return 15U;
    }
}

void LightEngine::rebuild(World& world) const {
    std::queue<BlockPos> propagation;

    world.for_each_chunk_mutable([&](Chunk& chunk) {
        chunk.clear_lighting();

        const auto chunk_pos = chunk.position();
        for (int local_x = 0; local_x < Chunk::width; ++local_x) {
            for (int local_z = 0; local_z < Chunk::depth; ++local_z) {
                std::uint8_t sky = 15U;

                for (int y = Chunk::height - 1; y >= 0; --y) {
                    const LocalBlockPos local{local_x, y, local_z};
                    const BlockState block = chunk.block_at(local);
                    const std::uint8_t opacity = opacity_for(block);

                    if (opacity != 0U) {
                        sky = static_cast<std::uint8_t>(
                            sky > opacity ? sky - opacity : 0U);
                    }
                    chunk.set_sky_light(local, sky);

                    const std::uint8_t emission = emission_for(block);
                    if (emission == 0U) {
                        continue;
                    }

                    chunk.set_block_light(local, emission);
                    propagation.push({
                        chunk_pos.x * Chunk::width + local_x,
                        y,
                        chunk_pos.z * Chunk::depth + local_z,
                    });
                }
            }
        }
    });

    while (!propagation.empty()) {
        const BlockPos current = propagation.front();
        propagation.pop();

        const std::uint8_t current_light = world.block_light_at(current);
        if (current_light <= 1U) {
            continue;
        }

        for (const auto& offset : neighbor_offsets) {
            const BlockPos neighbor = add(current, offset);
            if (!world.has_chunk_at(neighbor)) {
                continue;
            }

            const BlockState block = world.block_at(neighbor);
            const std::uint8_t opacity = opacity_for(block);
            if (opacity >= 15U) {
                continue;
            }

            const std::uint8_t attenuation = std::max<std::uint8_t>(1U, opacity);
            if (current_light <= attenuation) {
                continue;
            }
            const std::uint8_t candidate =
                static_cast<std::uint8_t>(current_light - attenuation);

            if (candidate <= world.block_light_at(neighbor)) {
                continue;
            }

            world.set_block_light(neighbor, candidate);
            propagation.push(neighbor);
        }
    }
}

void LightEngine::on_block_changed(
    World& world,
    const BlockPos& position,
    const BlockState& before,
    const BlockState& after) const {
    (void)position;
    (void)before;
    (void)after;
    rebuild(world);
}

} // namespace mcpi::world
