#pragma once

#include "world/World.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mcpi::game {

class GameState;

enum class TickStage {
    PlayerPhysics,
    ScheduledBlockUpdates,
};

class GameLoop {
public:
    // Provisional until an original Pi timing trace proves a different rate.
    static constexpr double provisional_tick_seconds = 1.0 / 20.0;
    static constexpr std::size_t max_catch_up_ticks = 20U;

    // Adds elapsed wall-clock time and executes zero or more fixed simulation
    // ticks. Returns the number executed by this call.
    std::size_t advance(GameState& game, double elapsed_seconds);

    void set_paused(bool paused) noexcept;
    [[nodiscard]] bool paused() const noexcept;
    [[nodiscard]] std::uint64_t tick_count() const noexcept;
    [[nodiscard]] double accumulator_seconds() const noexcept;

    [[nodiscard]] const std::vector<TickStage>& last_tick_trace() const noexcept;
    [[nodiscard]] const std::vector<world::BlockPos>& delivered_block_ticks() const noexcept;

private:
    void tick(GameState& game);

    double accumulator_seconds_ = 0.0;
    bool paused_ = false;
    std::uint64_t tick_count_ = 0;
    std::vector<TickStage> last_tick_trace_;
    std::vector<world::BlockPos> delivered_block_ticks_;
};

} // namespace mcpi::game
