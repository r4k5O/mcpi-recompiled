#pragma once

#include <cstddef>
#include <unordered_map>

namespace mcpi::world {

struct ChunkPos {
    int x = 0;
    int z = 0;

    bool operator==(const ChunkPos&) const = default;
};

struct LocalBlockPos {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const LocalBlockPos&) const = default;
};

struct BlockState {
    int id = 0;
    int data = 0;

    bool operator==(const BlockState&) const = default;
};

class Chunk {
public:
    explicit Chunk(ChunkPos position);

    [[nodiscard]] const ChunkPos& position() const noexcept;
    [[nodiscard]] BlockState block_at(const LocalBlockPos& position) const;
    void set_block(const LocalBlockPos& position, const BlockState& block);
    [[nodiscard]] bool empty() const noexcept;

private:
    struct LocalBlockPosHash {
        [[nodiscard]] std::size_t operator()(const LocalBlockPos& position) const noexcept;
    };

    ChunkPos position_;
    std::unordered_map<LocalBlockPos, BlockState, LocalBlockPosHash> blocks_;
};

} // namespace mcpi::world
