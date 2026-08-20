#include "client/ChunkMesh.hpp"

#include "world/Chunk.hpp"

#include <algorithm>
#include <array>

namespace mcpi::client {
namespace {

struct FaceDefinition {
    int nx;
    int ny;
    int nz;
    std::array<std::array<float, 3>, 4> corners;
};

constexpr std::array<FaceDefinition, 6> faces{{
    { 1, 0, 0, {{{1,0,0},{1,1,0},{1,1,1},{1,0,1}}}},
    {-1, 0, 0, {{{0,0,1},{0,1,1},{0,1,0},{0,0,0}}}},
    { 0, 1, 0, {{{0,1,0},{0,1,1},{1,1,1},{1,1,0}}}},
    { 0,-1, 0, {{{0,0,1},{0,0,0},{1,0,0},{1,0,1}}}},
    { 0, 0, 1, {{{1,0,1},{1,1,1},{0,1,1},{0,0,1}}}},
    { 0, 0,-1, {{{0,0,0},{0,1,0},{1,1,0},{1,0,0}}}},
}};

constexpr std::array<std::array<float, 2>, 4> quad_uv{{
    {{0.0f, 1.0f}}, {{0.0f, 0.0f}}, {{1.0f, 0.0f}}, {{1.0f, 1.0f}},
}};

bool visible_face(int current_id, int neighbor_id) {
    if (neighbor_id == 0) {
        return true;
    }
    const bool current_translucent = ChunkMeshBuilder::translucent(current_id);
    const bool neighbor_translucent = ChunkMeshBuilder::translucent(neighbor_id);
    if (!current_translucent) {
        return neighbor_translucent;
    }
    return !neighbor_translucent || neighbor_id != current_id;
}

void emit_face(std::vector<MeshVertex>& vertices,
               int x,
               int y,
               int z,
               int id,
               int data,
               std::size_t face_index,
               float light) {
    const int tile = (id * 17 + data * 5 + static_cast<int>(face_index)) & 255;
    const float tile_u = static_cast<float>(tile & 15) / 16.0f;
    const float tile_v = static_cast<float>((tile >> 4) & 15) / 16.0f;
    constexpr float tile_size = 1.0f / 16.0f;
    constexpr std::array<int, 6> order{{0, 1, 2, 0, 2, 3}};

    for (int index : order) {
        const auto& corner = faces[face_index].corners[static_cast<std::size_t>(index)];
        const auto& uv = quad_uv[static_cast<std::size_t>(index)];
        vertices.push_back({
            static_cast<float>(x) + corner[0],
            static_cast<float>(y) + corner[1],
            static_cast<float>(z) + corner[2],
            tile_u + uv[0] * tile_size,
            tile_v + uv[1] * tile_size,
            light,
        });
    }
}

} // namespace

bool ChunkMeshBuilder::translucent(int block_id) noexcept {
    switch (block_id) {
    case 8:
    case 9:
    case 18:
    case 20:
        return true;
    default:
        return false;
    }
}

ChunkMesh ChunkMeshBuilder::build(const world::World& world, int chunk_x, int chunk_z) const {
    ChunkMesh mesh;
    const int min_x = chunk_x * world::Chunk::width;
    const int min_z = chunk_z * world::Chunk::depth;

    for (int local_x = 0; local_x < world::Chunk::width; ++local_x) {
        const int x = min_x + local_x;
        for (int local_z = 0; local_z < world::Chunk::depth; ++local_z) {
            const int z = min_z + local_z;
            for (int y = 0; y < world::Chunk::height; ++y) {
                const world::BlockPos position{x, y, z};
                const auto block = world.block_at(position);
                if (block.id == 0) {
                    continue;
                }

                const auto sky = world.sky_light_at(position);
                const auto emitted = world.block_light_at(position);
                const float light = static_cast<float>(std::max(sky, emitted)) / 15.0f;
                auto& output = translucent(block.id) ? mesh.translucent : mesh.opaque;

                for (std::size_t face_index = 0; face_index < faces.size(); ++face_index) {
                    const auto& face = faces[face_index];
                    const int neighbor_id = world.block_at({x + face.nx, y + face.ny, z + face.nz}).id;
                    if (visible_face(block.id, neighbor_id)) {
                        emit_face(output, x, y, z, block.id, block.data, face_index, light);
                    }
                }
            }
        }
    }

    return mesh;
}

} // namespace mcpi::client
