#include "world/Chunk.hpp"

#include <functional>

namespace mcpi::world {

namespace {

std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

} // namespace

Chunk::Chunk(ChunkPos position)
    : position_(position) {}

const ChunkPos& Chunk::position() const noexcept {
    return position_;
}

std::size_t Chunk::LocalBlockPosHash::operator()(const LocalBlockPos& position) const noexcept {
    std::size_t seed = std::hash<int>{}(position.x);
    seed = hash_combine(seed, std::hash<int>{}(position.y));
    seed = hash_combine(seed, std::hash<int>{}(position.z));
    return seed;
}

BlockState Chunk::block_at(const LocalBlockPos& position) const {
    const auto found = blocks_.find(position);
    if (found == blocks_.end()) {
        return {};
    }
    return found->second;
}

void Chunk::set_block(const LocalBlockPos& position, const BlockState& block) {
    if (block.id == 0 && block.data == 0) {
        blocks_.erase(position);
        return;
    }
    blocks_.insert_or_assign(position, block);
}

bool Chunk::empty() const noexcept {
    return blocks_.empty();
}

} // namespace mcpi::world
