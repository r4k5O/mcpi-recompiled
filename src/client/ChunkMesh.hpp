#pragma once

#include "world/World.hpp"

#include <vector>

namespace mcpi::client {

struct MeshVertex {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float light = 1.0f;
};

struct ChunkMesh {
    std::vector<MeshVertex> opaque;
    std::vector<MeshVertex> translucent;
};

class ChunkMeshBuilder {
public:
    [[nodiscard]] ChunkMesh build(const world::World& world, int chunk_x, int chunk_z) const;

    [[nodiscard]] static bool translucent(int block_id) noexcept;
};

} // namespace mcpi::client
