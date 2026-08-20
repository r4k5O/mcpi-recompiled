#include "game/Entity.hpp"
#include "game/Physics.hpp"
#include "game/Player.hpp"
#include "world/World.hpp"

#include <cassert>
#include <cmath>

namespace {

bool close(double a, double b, double epsilon = 1.0e-6) {
    return std::abs(a - b) <= epsilon;
}

void set_solid(mcpi::world::World& world, int x, int y, int z) {
    world.set_block({x, y, z}, {1, 0});
}

void make_floor(mcpi::world::World& world, int y,
                int min_x, int max_x, int min_z, int max_z) {
    for (int x = min_x; x <= max_x; ++x) {
        for (int z = min_z; z <= max_z; ++z) {
            set_solid(world, x, y, z);
        }
    }
}

} // namespace

int main() {
    using mcpi::game::Aabb;
    using mcpi::game::Physics;
    using mcpi::game::Player;
    using mcpi::game::Velocity;
    using mcpi::world::World;

    World world;
    make_floor(world, 0, 0, 10, 0, 10);

    Player floor_player;
    floor_player.set_position({2.5, 3.0, 2.5});
    Physics::move(world, floor_player, {0.0, -10.0, 0.0});
    assert(close(floor_player.position().y, 1.0));
    assert(floor_player.on_ground());
    assert(!Physics::intersects_solid(world, floor_player.bounds()));

    set_solid(world, 4, 1, 2);
    set_solid(world, 4, 2, 2);
    Player wall_player;
    wall_player.set_position({2.5, 1.0, 2.5});
    Physics::move(world, wall_player, {5.0, 0.0, 0.0});
    assert(close(wall_player.position().x, 3.7));
    assert(!Physics::intersects_solid(world, wall_player.bounds()));

    set_solid(world, 6, 3, 6);
    Player ceiling_player;
    ceiling_player.set_position({6.5, 1.0, 6.5});
    Physics::move(world, ceiling_player, {0.0, 5.0, 0.0});
    assert(close(ceiling_player.position().y, 1.2));
    assert(!Physics::intersects_solid(world, ceiling_player.bounds()));

    Player edge_player;
    edge_player.set_position({1.0, 10.0, 1.0});
    Physics::move(world, edge_player, {-100.0, 0.0, -100.0});
    assert(edge_player.position().x >= Player::half_width);
    assert(edge_player.position().z >= Player::half_width);
    Physics::move(world, edge_player, {1000.0, 0.0, 1000.0});
    assert(edge_player.position().x <= 256.0 - Player::half_width);
    assert(edge_player.position().z <= 256.0 - Player::half_width);

    Player falling;
    falling.set_position({8.5, 8.0, 8.5});
    const double initial_y = falling.position().y;
    Physics::tick_player(world, falling);
    assert(falling.position().y < initial_y);
    for (int tick = 0; tick < 200 && !falling.on_ground(); ++tick) {
        Physics::tick_player(world, falling);
    }
    assert(falling.on_ground());
    assert(close(falling.position().y, 1.0));
    assert(close(falling.velocity().y, 0.0));
    assert(!Physics::intersects_solid(world, falling.bounds()));

    falling.jump();
    Physics::tick_player(world, falling);
    assert(falling.position().y > 1.0);
    assert(!falling.on_ground());
    for (int tick = 0; tick < 200 && !falling.on_ground(); ++tick) {
        Physics::tick_player(world, falling);
    }
    assert(falling.on_ground());
    assert(close(falling.position().y, 1.0));

    Player repeated;
    repeated.set_position({2.5, 1.0, 2.5});
    for (int step = 0; step < 100; ++step) {
        Physics::move(world, repeated, {0.25, 0.0, 0.0});
        assert(!Physics::intersects_solid(world, repeated.bounds()));
    }
    assert(close(repeated.position().x, 3.7));

    const Velocity expected_velocity{1.0, 2.0, 3.0};
    repeated.set_velocity(expected_velocity);
    assert(repeated.velocity() == expected_velocity);
    const Aabb bounds = repeated.bounds();
    assert(close(bounds.max.x - bounds.min.x, Player::half_width * 2.0));
    assert(close(bounds.max.y - bounds.min.y, Player::height));

    return 0;
}
