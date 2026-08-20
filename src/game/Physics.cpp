#include "game/Physics.hpp"

#include "world/BlockBehavior.hpp"

#include <algorithm>
#include <cmath>

namespace mcpi::game {
namespace {

constexpr double world_width = 256.0;
constexpr double world_height = 128.0;
constexpr double epsilon = 1.0e-9;

Aabb block_bounds(int x, int y, int z) noexcept {
    return {
        {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)},
        {static_cast<double>(x + 1), static_cast<double>(y + 1), static_cast<double>(z + 1)},
    };
}

bool overlaps(double a_min, double a_max,
              double b_min, double b_max) noexcept {
    return a_max > b_min + epsilon && a_min < b_max - epsilon;
}

bool is_solid(const world::World& world, int x, int y, int z) noexcept {
    if (x < 0 || x > 255 || y < 0 || y > 127 || z < 0 || z > 255) {
        return false;
    }
    const int id = world.block_at({x, y, z}).id;
    return world::BlockBehaviorRegistry::behavior(id).solid;
}

int first_block(double coordinate) noexcept {
    return static_cast<int>(std::floor(coordinate + epsilon));
}

int last_block(double coordinate) noexcept {
    return static_cast<int>(std::floor(coordinate - epsilon));
}

double clip_x(const world::World& world, const Aabb& box, double delta) noexcept {
    if (delta == 0.0) {
        return 0.0;
    }

    const double bounded = std::clamp(delta, -box.min.x, world_width - box.max.x);
    double clipped = bounded;

    const double swept_min = std::min(box.min.x, box.min.x + bounded);
    const double swept_max = std::max(box.max.x, box.max.x + bounded);
    const int min_x = std::max(0, first_block(swept_min));
    const int max_x = std::min(255, last_block(swept_max));
    const int min_y = std::max(0, first_block(box.min.y));
    const int max_y = std::min(127, last_block(box.max.y));
    const int min_z = std::max(0, first_block(box.min.z));
    const int max_z = std::min(255, last_block(box.max.z));

    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            for (int z = min_z; z <= max_z; ++z) {
                if (!is_solid(world, x, y, z)) {
                    continue;
                }
                const Aabb block = block_bounds(x, y, z);
                if (!overlaps(box.min.y, box.max.y, block.min.y, block.max.y) ||
                    !overlaps(box.min.z, box.max.z, block.min.z, block.max.z)) {
                    continue;
                }

                if (clipped > 0.0 && box.max.x <= block.min.x + epsilon) {
                    clipped = std::min(clipped, block.min.x - box.max.x);
                } else if (clipped < 0.0 && box.min.x >= block.max.x - epsilon) {
                    clipped = std::max(clipped, block.max.x - box.min.x);
                }
            }
        }
    }
    return clipped;
}

double clip_y(const world::World& world, const Aabb& box, double delta) noexcept {
    if (delta == 0.0) {
        return 0.0;
    }

    const double bounded = std::clamp(delta, -box.min.y, world_height - box.max.y);
    double clipped = bounded;

    const double swept_min = std::min(box.min.y, box.min.y + bounded);
    const double swept_max = std::max(box.max.y, box.max.y + bounded);
    const int min_x = std::max(0, first_block(box.min.x));
    const int max_x = std::min(255, last_block(box.max.x));
    const int min_y = std::max(0, first_block(swept_min));
    const int max_y = std::min(127, last_block(swept_max));
    const int min_z = std::max(0, first_block(box.min.z));
    const int max_z = std::min(255, last_block(box.max.z));

    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            for (int z = min_z; z <= max_z; ++z) {
                if (!is_solid(world, x, y, z)) {
                    continue;
                }
                const Aabb block = block_bounds(x, y, z);
                if (!overlaps(box.min.x, box.max.x, block.min.x, block.max.x) ||
                    !overlaps(box.min.z, box.max.z, block.min.z, block.max.z)) {
                    continue;
                }

                if (clipped > 0.0 && box.max.y <= block.min.y + epsilon) {
                    clipped = std::min(clipped, block.min.y - box.max.y);
                } else if (clipped < 0.0 && box.min.y >= block.max.y - epsilon) {
                    clipped = std::max(clipped, block.max.y - box.min.y);
                }
            }
        }
    }
    return clipped;
}

double clip_z(const world::World& world, const Aabb& box, double delta) noexcept {
    if (delta == 0.0) {
        return 0.0;
    }

    const double bounded = std::clamp(delta, -box.min.z, world_width - box.max.z);
    double clipped = bounded;

    const double swept_min = std::min(box.min.z, box.min.z + bounded);
    const double swept_max = std::max(box.max.z, box.max.z + bounded);
    const int min_x = std::max(0, first_block(box.min.x));
    const int max_x = std::min(255, last_block(box.max.x));
    const int min_y = std::max(0, first_block(box.min.y));
    const int max_y = std::min(127, last_block(box.max.y));
    const int min_z = std::max(0, first_block(swept_min));
    const int max_z = std::min(255, last_block(swept_max));

    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            for (int z = min_z; z <= max_z; ++z) {
                if (!is_solid(world, x, y, z)) {
                    continue;
                }
                const Aabb block = block_bounds(x, y, z);
                if (!overlaps(box.min.x, box.max.x, block.min.x, block.max.x) ||
                    !overlaps(box.min.y, box.max.y, block.min.y, block.max.y)) {
                    continue;
                }

                if (clipped > 0.0 && box.max.z <= block.min.z + epsilon) {
                    clipped = std::min(clipped, block.min.z - box.max.z);
                } else if (clipped < 0.0 && box.min.z >= block.max.z - epsilon) {
                    clipped = std::max(clipped, block.max.z - box.min.z);
                }
            }
        }
    }
    return clipped;
}

} // namespace

void Physics::move(
    const world::World& world,
    Entity& entity,
    const Vec3& desired_delta) noexcept {
    Vec3 position = entity.position();
    Velocity velocity = entity.velocity();

    Aabb box = entity.bounds();
    const double moved_y = clip_y(world, box, desired_delta.y);
    position.y += moved_y;
    entity.set_position(position);
    const bool hit_vertical = std::abs(moved_y - desired_delta.y) > epsilon;
    if (hit_vertical) {
        velocity.y = 0.0;
    }
    if (desired_delta.y < 0.0) {
        entity.set_on_ground(hit_vertical);
    } else if (desired_delta.y > 0.0) {
        entity.set_on_ground(false);
    }

    box = entity.bounds();
    const double moved_x = clip_x(world, box, desired_delta.x);
    position.x += moved_x;
    entity.set_position(position);
    if (std::abs(moved_x - desired_delta.x) > epsilon) {
        velocity.x = 0.0;
    }

    box = entity.bounds();
    const double moved_z = clip_z(world, box, desired_delta.z);
    position.z += moved_z;
    entity.set_position(position);
    if (std::abs(moved_z - desired_delta.z) > epsilon) {
        velocity.z = 0.0;
    }

    entity.set_velocity(velocity);
}

void Physics::tick_player(
    const world::World& world,
    Player& player) noexcept {
    Velocity velocity = player.velocity();
    velocity.y -= gravity_per_tick;
    player.set_velocity(velocity);

    move(world, player, {velocity.x, velocity.y, velocity.z});

    velocity = player.velocity();
    velocity.x *= horizontal_drag;
    velocity.z *= horizontal_drag;
    if (!player.on_ground()) {
        velocity.y *= vertical_drag;
    } else {
        velocity.y = 0.0;
    }
    player.set_velocity(velocity);
}

bool Physics::intersects_solid(
    const world::World& world,
    const Aabb& bounds) noexcept {
    if (bounds.min.x < -epsilon || bounds.max.x > world_width + epsilon ||
        bounds.min.y < -epsilon || bounds.max.y > world_height + epsilon ||
        bounds.min.z < -epsilon || bounds.max.z > world_width + epsilon) {
        return true;
    }

    const int min_x = std::max(0, first_block(bounds.min.x));
    const int max_x = std::min(255, last_block(bounds.max.x));
    const int min_y = std::max(0, first_block(bounds.min.y));
    const int max_y = std::min(127, last_block(bounds.max.y));
    const int min_z = std::max(0, first_block(bounds.min.z));
    const int max_z = std::min(255, last_block(bounds.max.z));

    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            for (int z = min_z; z <= max_z; ++z) {
                if (!is_solid(world, x, y, z)) {
                    continue;
                }
                const Aabb block = block_bounds(x, y, z);
                if (overlaps(bounds.min.x, bounds.max.x, block.min.x, block.max.x) &&
                    overlaps(bounds.min.y, bounds.max.y, block.min.y, block.max.y) &&
                    overlaps(bounds.min.z, bounds.max.z, block.min.z, block.max.z)) {
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace mcpi::game
