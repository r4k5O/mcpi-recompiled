#include "game/Player.hpp"

namespace mcpi::game {

Player::Player(int id) noexcept
    : Entity(id, half_width * 2.0, height) {}

Inventory& Player::inventory() noexcept {
    return inventory_;
}

const Inventory& Player::inventory() const noexcept {
    return inventory_;
}

void Player::jump() noexcept {
    if (!on_ground()) {
        return;
    }

    auto next_velocity = velocity();
    next_velocity.y = jump_velocity;
    set_velocity(next_velocity);
    set_on_ground(false);
}

} // namespace mcpi::game
