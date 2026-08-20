#include "world/BlockBehavior.hpp"
#include "world/World.hpp"

#include <cassert>
#include <cstddef>

int main() {
    using mcpi::world::BlockBehaviorRegistry;
    using mcpi::world::BlockPos;
    using mcpi::world::BlockState;
    using mcpi::world::BlockUpdateEngine;
    using mcpi::world::World;

    const auto& air = BlockBehaviorRegistry::behavior(0);
    assert(air.opacity == 0);
    assert(air.emission == 0);
    assert(!air.solid);
    assert(air.replaceable);
    assert(!air.scheduled_tick);

    const auto& stone = BlockBehaviorRegistry::behavior(1);
    assert(stone.opacity == 15);
    assert(stone.emission == 0);
    assert(stone.solid);
    assert(!stone.replaceable);

    const auto& glass = BlockBehaviorRegistry::behavior(20);
    assert(glass.opacity == 0);
    assert(glass.solid);
    assert(!glass.replaceable);

    const auto& water = BlockBehaviorRegistry::behavior(8);
    assert(water.opacity == 2);
    assert(!water.solid);
    assert(water.replaceable);
    assert(water.scheduled_tick);

    const auto& glowstone = BlockBehaviorRegistry::behavior(89);
    assert(glowstone.emission == 15);
    assert(glowstone.solid);

    const auto& unknown = BlockBehaviorRegistry::behavior(255);
    assert(unknown.opacity == 15);
    assert(unknown.emission == 0);
    assert(unknown.solid);
    assert(!unknown.replaceable);
    assert(!unknown.scheduled_tick);

    // Data values belong to block state and must survive behavior lookup/write paths.
    World world;
    const BlockPos wool_position{1, 4, 1};
    world.set_block(wool_position, BlockState{35, 7});
    assert(world.block_at(wool_position) == (BlockState{35, 7}));
    (void)BlockBehaviorRegistry::behavior(world.block_at(wool_position).id);
    assert(world.block_at(wool_position).data == 7);

    BlockUpdateEngine updates;
    updates.on_block_changed(world, wool_position, BlockState{0, 0}, BlockState{35, 7});
    assert(updates.neighbor_notification_count() == 6U);

    // The scheduled queue must remain bounded even when a caller floods it.
    for (std::size_t index = 0; index < BlockUpdateEngine::max_scheduled_ticks + 128U; ++index) {
        updates.schedule_tick(BlockPos{static_cast<int>(index), 5, 0});
    }
    assert(updates.scheduled_tick_count() == BlockUpdateEngine::max_scheduled_ticks);

    BlockPos scheduled{};
    assert(updates.pop_scheduled_tick(scheduled));
    assert(updates.scheduled_tick_count() == BlockUpdateEngine::max_scheduled_ticks - 1U);

    updates.clear();
    assert(updates.neighbor_notification_count() == 0U);
    assert(updates.scheduled_tick_count() == 0U);

    return 0;
}
