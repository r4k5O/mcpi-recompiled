#include "world/World.hpp"

#include <functional>

namespace mcpi::world {

namespace {

constexpr int chunk_width = 16;

std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

int floor_divide_by_chunk_width(int value) noexcept {
    int quotient = value / chunk_width;
    const int remainder = value % chunk_width;
    if (remainder < 0) {
        --quotient;
    }
    return quotient;
}

int local_coordinate(int value) noexcept {
    int remainder = value % chunk_width;
    if (remainder < 0) {
        remainder += chunk_width;
    }
    return remainder;
}

} // namespace

std::size_t World::ChunkPosHash::operator()(const ChunkPos& position) const noexcept {
    std::size_t seed = std::hash<int>{}(position.x);
    seed = hash_combine(seed, std::hash<int>{}(position.z));
    return seed;
}

ChunkPos World::chunk_position(const BlockPos& position) noexcept {
    return {
        floor_divide_by_chunk_width(position.x),
        floor_divide_by_chunk_width(position.z),
    };
}

LocalBlockPos World::local_position(const BlockPos& position) noexcept {
    return {
        local_coordinate(position.x),
        position.y,
        local_coordinate(position.z),
    };
}

BlockState World::block_at(const BlockPos& position) const {
    const ChunkPos chunk_pos = chunk_position(position);
    const auto found = chunks_.find(chunk_pos);
    if (found == chunks_.end()) {
        return {};
    }
    return found->second.block_at(local_position(position));
}

void World::set_block(const BlockPos& position, const BlockState& block) {
    const ChunkPos chunk_pos = chunk_position(position);
    auto found = chunks_.find(chunk_pos);

    if (found == chunks_.end()) {
        if (block.id == 0 && block.data == 0) {
            return;
        }
        found = chunks_.try_emplace(chunk_pos, chunk_pos).first;
    }

    found->second.set_block(local_position(position), block);
    if (found->second.empty()) {
        chunks_.erase(found);
    }
}

std::size_t World::chunk_count() const noexcept {
    return chunks_.size();
}

} // namespace mcpi::world
