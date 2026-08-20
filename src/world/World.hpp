#pragma once

#include "world/Chunk.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
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
    [[nodiscard]] BlockState block_at(const BlockPos& position) const noexcept;
    void set_block(const BlockPos& position, const BlockState& block);
    [[nodiscard]] int height_at(int x, int z) const noexcept;

    [[nodiscard]] std::uint8_t sky_light_at(const BlockPos& position) const noexcept;
    [[nodiscard]] std::uint8_t block_light_at(const BlockPos& position) const noexcept;
    void set_sky_light(const BlockPos& position, std::uint8_t light) noexcept;
    void set_block_light(const BlockPos& position, std::uint8_t light) noexcept;
    [[nodiscard]] bool has_chunk_at(const BlockPos& position) const noexcept;

    [[nodiscard]] std::size_t chunk_count() const noexcept;
    void clear() noexcept;

    void for_each_chunk(const std::function<void(const Chunk&)>& visitor) const;
    void for_each_chunk_mutable(const std::function<void(Chunk&)>& visitor);

private:
    struct ChunkPosHash {
        [[nodiscard]] std::size_t operator()(const ChunkPos& position) const noexcept;
    };

    [[nodiscard]] static ChunkPos chunk_position(const BlockPos& position) noexcept;
    [[nodiscard]] static LocalBlockPos local_position(const BlockPos& position) noexcept;
    [[nodiscard]] static int local_coordinate(int value) noexcept;
    [[nodiscard]] static int floor_divide_by_chunk_width(int value) noexcept;

    std::unordered_map<ChunkPos, Chunk, ChunkPosHash> chunks_;
};

} // namespace mcpi::world
