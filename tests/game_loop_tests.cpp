#include "game/GameLoop.hpp"
#include "game/GameState.hpp"

#include <cassert>
#include <vector>

int main() {
    using mcpi::game::GameLoop;
    using mcpi::game::GameState;
    using mcpi::game::TickStage;
    using mcpi::world::BlockPos;

    GameState game;
    game.new_world(42U);
    game.world().clear();

    // Build a deterministic floor and put the player above it so a simulation
    // tick has observable physics work to do.
    for (int x = 0; x < 8; ++x) {
        for (int z = 0; z < 8; ++z) {
            game.set_block(x, 0, z, 1, 0);
        }
    }
    game.set_player_position({2.5, 3.0, 2.5});

    GameLoop loop;
    assert(loop.tick_count() == 0U);
    assert(!loop.paused());

    // Frame duration and simulation tick duration are independent. Three and
    // a half ticks worth of elapsed time must execute exactly three ticks and
    // preserve the remainder for the next advance.
    const double dt = GameLoop::provisional_tick_seconds;
    assert(loop.advance(game, dt * 3.5) == 3U);
    assert(loop.tick_count() == 3U);
    assert(game.player_position().y < 3.0);
    assert((loop.last_tick_trace() ==
            std::vector<TickStage>{TickStage::PlayerPhysics,
                                   TickStage::ScheduledBlockUpdates}));

    assert(loop.advance(game, dt * 0.5) == 1U);
    assert(loop.tick_count() == 4U);

    // Pause must suppress simulation and must not accumulate a giant catch-up
    // burst for elapsed paused wall-clock time.
    loop.set_paused(true);
    assert(loop.paused());
    assert(loop.advance(game, 10.0) == 0U);
    assert(loop.tick_count() == 4U);
    loop.set_paused(false);
    assert(loop.advance(game, dt) == 1U);
    assert(loop.tick_count() == 5U);

    // Water is currently one of the evidence-bounded scheduled-tick block
    // classes. GameLoop only delivers the queued position; it deliberately
    // does not invent original liquid behavior yet.
    const BlockPos scheduled{5, 5, 5};
    game.set_block(scheduled.x, scheduled.y, scheduled.z, 8, 0);
    assert(game.scheduled_block_tick_count() >= 1U);
    assert(loop.advance(game, dt) == 1U);
    assert(game.scheduled_block_tick_count() == 0U);
    assert((loop.delivered_block_ticks() == std::vector<BlockPos>{scheduled}));
    assert((loop.last_tick_trace() ==
            std::vector<TickStage>{TickStage::PlayerPhysics,
                                   TickStage::ScheduledBlockUpdates}));

    // Invalid elapsed values are ignored rather than destabilizing the
    // accumulator.
    assert(loop.advance(game, -1.0) == 0U);
    assert(loop.tick_count() == 6U);

    return 0;
}
