#include "world/Chunk.hpp"

#include <algorithm>

namespace mcpi::world {

Chunk::Chunk(ChunkPos position)
    : position_(position) {}

const ChunkPos& Chunk::position() const noexcept {
    return position_;
}

bool Chunk::valid(const LocalBlockPos& position) noexcept {
    return position.x >= 0 && position.x < width &&
           position.y >= 0 && position.y < height &&
           position.z >= 0 && position.z < depth;
}

bool Chunk::valid_column(int x, int z) noexcept {
    return x >= 0 && x < width && z >= 0 && z < depth;
}

std::size_t Chunk::block_index(const LocalBlockPos& position) noexcept {
    // Confirmed MCPI 0.1.1 LevelChunk layout:
    // (x << 11) | (z << 7) | y == x*2048 + z*128 + y.
    return (static_cast<std::size_t>(position.x) << 11U) |
           (static_cast<std::size_t>(position.z) << 7U) |
           static_cast<std::size_t>(position.y);
}

std::size_t Chunk::column_index(int x, int z) noexcept {
    return static_cast<std::size_t>(x | (z << 4));
}

std::uint8_t Chunk::nibble_at(
    const std::array<std::uint8_t, nibble_storage_size>& storage,
    std::size_t index) noexcept {
    const std::uint8_t packed = storage[index >> 1U];
    return static_cast<std::uint8_t>(
        (index & 1U) == 0U ? (packed & 0x0fU) : ((packed >> 4U) & 0x0fU));
}

void Chunk::set_nibble(
    std::array<std::uint8_t, nibble_storage_size>& storage,
    std::size_t index,
    std::uint8_t value) noexcept {
    value = static_cast<std::uint8_t>(value & 0x0fU);
    auto& packed = storage[index >> 1U];
    if ((index & 1U) == 0U) {
        packed = static_cast<std::uint8_t>((packed & 0xf0U) | value);
    } else {
        packed = static_cast<std::uint8_t>((packed & 0x0fU) | (value << 4U));
    }
}

BlockState Chunk::block_at(const LocalBlockPos& position) const noexcept {
    if (!valid(position)) {
        return {};
    }

    const auto index = block_index(position);
    return {
        static_cast<int>(block_ids_[index]),
        static_cast<int>(nibble_at(metadata_, index)),
    };
}

void Chunk::recompute_height(int local_x, int local_z, int start_y) noexcept {
    if (!valid_column(local_x, local_z)) {
        return;
    }

    int y = std::min(start_y, height - 1);
    for (; y >= 0; --y) {
        const LocalBlockPos position{local_x, y, local_z};
        if (block_ids_[block_index(position)] != 0U) {
            height_map_[column_index(local_x, local_z)] =
                static_cast<std::uint8_t>(y + 1);
            return;
        }
    }

    height_map_[column_index(local_x, local_z)] = 0U;
}

void Chunk::set_block(const LocalBlockPos& position, const BlockState& block) noexcept {
    if (!valid(position)) {
        return;
    }

    const auto index = block_index(position);
    const std::uint8_t old_id = block_ids_[index];
    const std::uint8_t new_id = static_cast<std::uint8_t>(block.id & 0xff);

    if (old_id == 0U && new_id != 0U) {
        ++non_air_blocks_;
    } else if (old_id != 0U && new_id == 0U) {
        --non_air_blocks_;
    }

    block_ids_[index] = new_id;
    set_nibble(metadata_, index, static_cast<std::uint8_t>(block.data));

    const auto column = column_index(position.x, position.z);
    if (new_id != 0U) {
        const int candidate_height = position.y + 1;
        if (candidate_height > static_cast<int>(height_map_[column])) {
            height_map_[column] = static_cast<std::uint8_t>(candidate_height);
        }
    } else if (old_id != 0U &&
               static_cast<int>(height_map_[column]) == position.y + 1) {
        recompute_height(position.x, position.z, position.y - 1);
    }
}

std::uint8_t Chunk::sky_light_at(const LocalBlockPos& position) const noexcept {
    if (!valid(position)) {
        return 0U;
    }
    return nibble_at(sky_light_, block_index(position));
}

void Chunk::set_sky_light(const LocalBlockPos& position, std::uint8_t light) noexcept {
    if (!valid(position)) {
        return;
    }
    set_nibble(sky_light_, block_index(position), light);
}

std::uint8_t Chunk::block_light_at(const LocalBlockPos& position) const noexcept {
    if (!valid(position)) {
        return 0U;
    }
    return nibble_at(block_light_, block_index(position));
}

void Chunk::set_block_light(const LocalBlockPos& position, std::uint8_t light) noexcept {
    if (!valid(position)) {
        return;
    }
    set_nibble(block_light_, block_index(position), light);
}

void Chunk::clear_lighting() noexcept {
    sky_light_.fill(0U);
    block_light_.fill(0U);
}

int Chunk::height_at(int local_x, int local_z) const noexcept {
    if (!valid_column(local_x, local_z)) {
        return 0;
    }
    return static_cast<int>(height_map_[column_index(local_x, local_z)]);
}

bool Chunk::empty() const noexcept {
    return non_air_blocks_ == 0U;
}

} // namespace mcpi::world
