#include "game/GameState.hpp"

namespace mcpi::game {

std::size_t GameState::scheduled_block_tick_count() const noexcept {
    return block_updates_.scheduled_tick_count();
}

bool GameState::deliver_scheduled_block_tick(world::BlockPos& position) noexcept {
    return block_updates_.pop_scheduled_tick(position);
}

} // namespace mcpi::game
