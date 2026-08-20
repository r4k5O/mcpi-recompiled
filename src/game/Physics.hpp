#pragma once

#include "game/Entity.hpp"
#include "game/Player.hpp"
#include "world/World.hpp"

namespace mcpi::game {

class Physics {
public:
    // These are deterministic reconstruction defaults. They are intentionally
    // not labelled exact-original until reference traces establish constants.
    static constexpr double gravity_per_tick = 0.08;
    static constexpr double vertical_drag = 0.98;
    static constexpr double horizontal_drag = 0.91;

    static void move(
        const world::World& world,
        Entity& entity,
        const Vec3& desired_delta) noexcept;

    static void tick_player(
        const world::World& world,
        Player& player) noexcept;

    [[nodiscard]] static bool intersects_solid(
        const world::World& world,
        const Aabb& bounds) noexcept;
};

} // namespace mcpi::game
