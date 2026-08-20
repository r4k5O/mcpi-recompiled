#include "world/LightEngine.hpp"
#include "world/World.hpp"

#include <cassert>

int main() {
    using mcpi::world::BlockPos;
    using mcpi::world::BlockState;
    using mcpi::world::LightEngine;
    using mcpi::world::World;

    LightEngine engine;

    // Open sky above an existing chunk must retain full skylight.
    World sky_world;
    sky_world.set_block({0, 0, 0}, {7, 0});
    engine.rebuild(sky_world);
    assert(sky_world.sky_light_at({0, 127, 0}) == 15U);
    assert(sky_world.sky_light_at({0, 1, 0}) == 15U);

    // An opaque roof must create a measurable shadow below it.
    for (int x = 0; x < 3; ++x) {
        for (int z = 0; z < 3; ++z) {
            sky_world.set_block({x, 10, z}, {1, 0});
        }
    }
    engine.rebuild(sky_world);
    assert(sky_world.sky_light_at({1, 9, 1}) < 15U);

    // Emissive block light attenuates one level per unobstructed step.
    World block_world;
    const BlockPos source{5, 5, 5};
    block_world.set_block({0, 0, 0}, {7, 0});
    block_world.set_block(source, {89, 0}); // glowstone
    engine.rebuild(block_world);
    assert(block_world.block_light_at(source) == 15U);
    assert(block_world.block_light_at({6, 5, 5}) == 14U);
    assert(block_world.block_light_at({7, 5, 5}) == 13U);

    // Removing the source must remove stale propagated block light.
    block_world.set_block(source, {0, 0});
    engine.on_block_changed(block_world, source, BlockState{89, 0}, BlockState{0, 0});
    assert(block_world.block_light_at(source) == 0U);
    assert(block_world.block_light_at({6, 5, 5}) == 0U);

    // Propagation must not stop at a 16-block chunk boundary.
    World boundary_world;
    boundary_world.set_block({15, 0, 0}, {7, 0});
    boundary_world.set_block({16, 0, 0}, {7, 0});
    boundary_world.set_block({15, 5, 0}, {89, 0});
    engine.rebuild(boundary_world);
    assert(boundary_world.block_light_at({15, 5, 0}) == 15U);
    assert(boundary_world.block_light_at({16, 5, 0}) == 14U);

    return 0;
}
