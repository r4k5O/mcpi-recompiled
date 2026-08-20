#pragma once

#include "world/ChunkSource.hpp"

#include <cstdint>

namespace mcpi::world {

class RandomLevelSource final : public ChunkSource {
public:
    explicit RandomLevelSource(std::uint32_t seed) noexcept;

    [[nodiscard]] Chunk generate_chunk(int chunk_x, int chunk_z) const override;

    // Temporary compatibility hook for the Phase-1 terrain profile. Keeping
    // this named explicitly prevents the placeholder algorithm from being
    // mistaken for reconstructed original RandomLevelSource behavior.
    [[nodiscard]] static int phase1_surface_height(
        std::uint32_t seed,
        int world_x,
        int world_z) noexcept;

private:
    [[nodiscard]] static std::uint32_t phase1_hash(
        std::uint32_t seed,
        int x,
        int z) noexcept;

    std::uint32_t seed_ = 0;
};

} // namespace mcpi::world
