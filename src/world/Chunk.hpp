#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

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
    static constexpr int width = 16;
    static constexpr int height = 128;
    static constexpr int depth = 16;
    static constexpr std::size_t block_count = 16U * 16U * 128U;
    static constexpr std::size_t nibble_storage_size = block_count / 2U;
    static constexpr std::size_t column_count = 16U * 16U;

    explicit Chunk(ChunkPos position);

    [[nodiscard]] const ChunkPos& position() const noexcept;
    [[nodiscard]] BlockState block_at(const LocalBlockPos& position) const noexcept;
    void set_block(const LocalBlockPos& position, const BlockState& block) noexcept;

    [[nodiscard]] std::uint8_t sky_light_at(const LocalBlockPos& position) const noexcept;
    void set_sky_light(const LocalBlockPos& position, std::uint8_t light) noexcept;
    [[nodiscard]] std::uint8_t block_light_at(const LocalBlockPos& position) const noexcept;
    void set_block_light(const LocalBlockPos& position, std::uint8_t light) noexcept;
    void clear_lighting() noexcept;

    [[nodiscard]] int height_at(int local_x, int local_z) const noexcept;
    [[nodiscard]] bool empty() const noexcept;

private:
    [[nodiscard]] static bool valid(const LocalBlockPos& position) noexcept;
    [[nodiscard]] static bool valid_column(int x, int z) noexcept;
    [[nodiscard]] static std::size_t block_index(const LocalBlockPos& position) noexcept;
    [[nodiscard]] static std::size_t column_index(int x, int z) noexcept;
    [[nodiscard]] static std::uint8_t nibble_at(
        const std::array<std::uint8_t, nibble_storage_size>& storage,
        std::size_t index) noexcept;
    static void set_nibble(
        std::array<std::uint8_t, nibble_storage_size>& storage,
        std::size_t index,
        std::uint8_t value) noexcept;

    void recompute_height(int local_x, int local_z, int start_y) noexcept;

    ChunkPos position_;
    std::array<std::uint8_t, block_count> block_ids_{};
    std::array<std::uint8_t, nibble_storage_size> metadata_{};
    std::array<std::uint8_t, nibble_storage_size> sky_light_{};
    std::array<std::uint8_t, nibble_storage_size> block_light_{};
    std::array<std::uint8_t, column_count> height_map_{};
    std::size_t non_air_blocks_ = 0;
};

} // namespace mcpi::world
