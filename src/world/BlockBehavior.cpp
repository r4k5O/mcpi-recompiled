#include "world/BlockBehavior.hpp"

namespace mcpi::world {
namespace {

constexpr BlockBehavior inert_solid{15, 0, true, false, false};
constexpr BlockBehavior air{0, 0, false, true, false};
constexpr BlockBehavior transparent_plant{0, 0, false, true, false};
constexpr BlockBehavior glass{0, 0, true, false, false};
constexpr BlockBehavior leaves{1, 0, true, false, false};
constexpr BlockBehavior water{2, 0, false, true, true};
constexpr BlockBehavior lava{15, 15, false, true, true};
constexpr BlockBehavior torch{0, 14, false, false, false};
constexpr BlockBehavior fire{0, 15, false, true, true};
constexpr BlockBehavior glowstone{15, 15, true, false, false};
constexpr BlockBehavior jack_o_lantern{15, 15, true, false, false};

} // namespace

const BlockBehavior& BlockBehaviorRegistry::behavior(int block_id) noexcept {
    switch (block_id) {
    case 0:
        return air;
    case 6:
    case 37:
    case 38:
    case 39:
    case 40:
        return transparent_plant;
    case 8:
    case 9:
        return water;
    case 10:
    case 11:
        return lava;
    case 18:
        return leaves;
    case 20:
        return glass;
    case 50:
        return torch;
    case 51:
        return fire;
    case 89:
        return glowstone;
    case 91:
        return jack_o_lantern;
    default:
        return inert_solid;
    }
}

void BlockUpdateEngine::on_block_changed(
    const World& world,
    const BlockPos& position,
    const BlockState& before,
    const BlockState& after) {
    (void)world;

    if (before == after) {
        return;
    }

    neighbor_notifications_ += 6U;
    if (BlockBehaviorRegistry::behavior(after.id).scheduled_tick) {
        schedule_tick(position);
    }
}

void BlockUpdateEngine::schedule_tick(const BlockPos& position) {
    if (scheduled_ticks_.size() >= max_scheduled_ticks) {
        return;
    }
    scheduled_ticks_.push_back(position);
}

bool BlockUpdateEngine::pop_scheduled_tick(BlockPos& position) noexcept {
    if (scheduled_ticks_.empty()) {
        return false;
    }

    position = scheduled_ticks_.front();
    scheduled_ticks_.pop_front();
    return true;
}

std::size_t BlockUpdateEngine::neighbor_notification_count() const noexcept {
    return neighbor_notifications_;
}

std::size_t BlockUpdateEngine::scheduled_tick_count() const noexcept {
    return scheduled_ticks_.size();
}

void BlockUpdateEngine::clear() noexcept {
    neighbor_notifications_ = 0U;
    scheduled_ticks_.clear();
}

} // namespace mcpi::world
