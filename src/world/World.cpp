#include "world/World.hpp"

#include <functional>

namespace mcpi::world {

namespace {

constexpr int chunk_width = 16;

std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

} // namespace

int World::floor_divide_by_chunk_width(int value) noexcept {
    int quotient = value / chunk_width;
    const int remainder = value % chunk_width;
    if (remainder < 0) {
        --quotient;
    }
    return quotient;
}

int World::local_coordinate(int value) noexcept {
    int remainder = value % chunk_width;
    if (remainder < 0) {
        remainder += chunk_width;
    }
    return remainder;
}

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

BlockState World::block_at(const BlockPos& position) const noexcept {
    if (position.y < 0 || position.y >= Chunk::height) {
        return {};
    }

    const ChunkPos chunk_pos = chunk_position(position);
    const auto found = chunks_.find(chunk_pos);
    if (found == chunks_.end()) {
        return {};
    }
    return found->second.block_at(local_position(position));
}

void World::set_block(const BlockPos& position, const BlockState& block) {
    if (position.y < 0 || position.y >= Chunk::height) {
        return;
    }

    const ChunkPos chunk_pos = chunk_position(position);
    auto found = chunks_.find(chunk_pos);

    if (found == chunks_.end()) {
        if (block.id == 0) {
            return;
        }
        found = chunks_.try_emplace(chunk_pos, chunk_pos).first;
    }

    found->second.set_block(local_position(position), block);
    if (found->second.empty()) {
        chunks_.erase(found);
    }
}

int World::height_at(int x, int z) const noexcept {
    const BlockPos position{x, 0, z};
    const ChunkPos chunk_pos = chunk_position(position);
    const auto found = chunks_.find(chunk_pos);
    if (found == chunks_.end()) {
        return 0;
    }
    return found->second.height_at(local_coordinate(x), local_coordinate(z));
}

std::uint8_t World::sky_light_at(const BlockPos& position) const noexcept {
    if (position.y < 0 || position.y >= Chunk::height) {
        return 0U;
    }
    const auto found = chunks_.find(chunk_position(position));
    if (found == chunks_.end()) {
        return 0U;
    }
    return found->second.sky_light_at(local_position(position));
}

std::uint8_t World::block_light_at(const BlockPos& position) const noexcept {
    if (position.y < 0 || position.y >= Chunk::height) {
        return 0U;
    }
    const auto found = chunks_.find(chunk_position(position));
    if (found == chunks_.end()) {
        return 0U;
    }
    return found->second.block_light_at(local_position(position));
}

void World::set_sky_light(const BlockPos& position, std::uint8_t light) noexcept {
    if (position.y < 0 || position.y >= Chunk::height) {
        return;
    }
    const auto found = chunks_.find(chunk_position(position));
    if (found == chunks_.end()) {
        return;
    }
    found->second.set_sky_light(local_position(position), light);
}

void World::set_block_light(const BlockPos& position, std::uint8_t light) noexcept {
    if (position.y < 0 || position.y >= Chunk::height) {
        return;
    }
    const auto found = chunks_.find(chunk_position(position));
    if (found == chunks_.end()) {
        return;
    }
    found->second.set_block_light(local_position(position), light);
}

bool World::has_chunk_at(const BlockPos& position) const noexcept {
    if (position.y < 0 || position.y >= Chunk::height) {
        return false;
    }
    return chunks_.find(chunk_position(position)) != chunks_.end();
}

std::size_t World::chunk_count() const noexcept {
    return chunks_.size();
}

void World::clear() noexcept {
    chunks_.clear();
}

void World::for_each_chunk(const std::function<void(const Chunk&)>& visitor) const {
    for (const auto& [position, chunk] : chunks_) {
        (void)position;
        visitor(chunk);
    }
}

void World::for_each_chunk_mutable(const std::function<void(Chunk&)>& visitor) {
    for (auto& [position, chunk] : chunks_) {
        (void)position;
        visitor(chunk);
    }
}

} // namespace mcpi::world
