#pragma once

#include "world/Chunk.hpp"

#include <cstddef>
#include <unordered_map>

namespace mcpi::world {

struct BlockPos {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const BlockPos&) const = default;
};

class World {
public:
    [[nodiscard]] BlockState block_at(const BlockPos& position) const;
    void set_block(const BlockPos& position, const BlockState& block);

    [[nodiscard]] std::size_t chunk_count() const noexcept;

private:
    struct ChunkPosHash {
        [[nodiscard]] std::size_t operator()(const ChunkPos& position) const noexcept;
    };

    [[nodiscard]] static ChunkPos chunk_position(const BlockPos& position) noexcept;
    [[nodiscard]] static LocalBlockPos local_position(const BlockPos& position) noexcept;

    std::unordered_map<ChunkPos, Chunk, ChunkPosHash> chunks_;
};

} // namespace mcpi::world
