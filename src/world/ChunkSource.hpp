#pragma once

#include "world/Chunk.hpp"

namespace mcpi::world {

class ChunkSource {
public:
    virtual ~ChunkSource() = default;

    [[nodiscard]] virtual Chunk generate_chunk(int chunk_x, int chunk_z) const = 0;
};

} // namespace mcpi::world
