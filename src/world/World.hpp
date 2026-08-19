#pragma once

#include <cstddef>
#include <unordered_map>

namespace mcpi::world {

struct BlockPos {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const BlockPos&) const = default;
};

struct BlockState {
    int id = 0;
    int data = 0;

    bool operator==(const BlockState&) const = default;
};

class World {
public:
    [[nodiscard]] BlockState block_at(const BlockPos& position) const;
    void set_block(const BlockPos& position, const BlockState& block);

private:
    struct BlockPosHash {
        [[nodiscard]] std::size_t operator()(const BlockPos& position) const noexcept;
    };

    std::unordered_map<BlockPos, BlockState, BlockPosHash> blocks_;
};

} // namespace mcpi::world
