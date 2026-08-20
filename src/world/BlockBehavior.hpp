#pragma once

#include "world/World.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>

namespace mcpi::world {

struct BlockBehavior {
    int opacity = 15;
    int emission = 0;
    bool solid = true;
    bool replaceable = false;
    bool scheduled_tick = false;
};

class BlockBehaviorRegistry {
public:
    // IDs without confirmed/required behavior deliberately receive a
    // conservative inert solid default rather than guessed semantics.
    [[nodiscard]] static const BlockBehavior& behavior(int block_id) noexcept;
};

class BlockUpdateEngine {
public:
    static constexpr std::size_t max_scheduled_ticks = 4096U;

    void on_block_changed(
        const World& world,
        const BlockPos& position,
        const BlockState& before,
        const BlockState& after);

    void schedule_tick(const BlockPos& position);
    [[nodiscard]] bool pop_scheduled_tick(BlockPos& position) noexcept;

    [[nodiscard]] std::size_t neighbor_notification_count() const noexcept;
    [[nodiscard]] std::size_t scheduled_tick_count() const noexcept;
    void clear() noexcept;

private:
    std::size_t neighbor_notifications_ = 0U;
    std::deque<BlockPos> scheduled_ticks_;
};

} // namespace mcpi::world
