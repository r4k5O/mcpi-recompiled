#include "game/GameLoop.hpp"

#include "game/GameState.hpp"
#include "game/Physics.hpp"

#include <algorithm>
#include <cmath>

namespace mcpi::game {

std::size_t GameLoop::advance(GameState& game, double elapsed_seconds) {
    if (paused_ || !std::isfinite(elapsed_seconds) || elapsed_seconds <= 0.0) {
        return 0U;
    }

    // Bound one frame's catch-up contribution. This prevents an arbitrarily
    // long stall from producing an unbounded simulation burst while still
    // preserving ordinary sub-tick remainder exactly in the accumulator.
    const double max_elapsed =
        provisional_tick_seconds * static_cast<double>(max_catch_up_ticks);
    accumulator_seconds_ += std::min(elapsed_seconds, max_elapsed);

    std::size_t executed = 0U;
    while (accumulator_seconds_ + 1.0e-12 >= provisional_tick_seconds &&
           executed < max_catch_up_ticks) {
        accumulator_seconds_ -= provisional_tick_seconds;
        if (accumulator_seconds_ < 0.0 && accumulator_seconds_ > -1.0e-12) {
            accumulator_seconds_ = 0.0;
        }
        tick(game);
        ++executed;
    }
    return executed;
}

void GameLoop::set_paused(bool paused) noexcept {
    paused_ = paused;
    if (paused_) {
        // Paused wall-clock time is intentionally discarded. Resuming must not
        // trigger a giant simulation catch-up burst.
        accumulator_seconds_ = 0.0;
    }
}

bool GameLoop::paused() const noexcept {
    return paused_;
}

std::uint64_t GameLoop::tick_count() const noexcept {
    return tick_count_;
}

double GameLoop::accumulator_seconds() const noexcept {
    return accumulator_seconds_;
}

const std::vector<TickStage>& GameLoop::last_tick_trace() const noexcept {
    return last_tick_trace_;
}

const std::vector<world::BlockPos>& GameLoop::delivered_block_ticks() const noexcept {
    return delivered_block_ticks_;
}

void GameLoop::tick(GameState& game) {
    last_tick_trace_.clear();
    delivered_block_ticks_.clear();

    last_tick_trace_.push_back(TickStage::PlayerPhysics);
    Physics::tick_player(game.world(), game.player());

    last_tick_trace_.push_back(TickStage::ScheduledBlockUpdates);
    world::BlockPos position;
    while (game.deliver_scheduled_block_tick(position)) {
        delivered_block_ticks_.push_back(position);
    }

    ++tick_count_;
}

} // namespace mcpi::game
