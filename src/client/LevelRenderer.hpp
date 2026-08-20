#pragma once

#include "client/Camera.hpp"
#include "client/ChunkMesh.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace mcpi::client {

struct RenderFrame {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;
};

class LevelRenderer {
public:
    [[nodiscard]] RenderFrame render(
        const world::World& world,
        const CameraPose& camera,
        int width,
        int height,
        int chunk_radius = 2,
        std::optional<world::BlockPos> selected = std::nullopt) const;

private:
    ChunkMeshBuilder mesh_builder_;
};

} // namespace mcpi::client
