#include "game/Entity.hpp"

namespace mcpi::game {

Entity::Entity(int id, double width, double height) noexcept
    : id_(id), width_(width), height_(height) {}

int Entity::id() const noexcept {
    return id_;
}

const Vec3& Entity::position() const noexcept {
    return position_;
}

void Entity::set_position(const Vec3& position) noexcept {
    position_ = position;
}

const Velocity& Entity::velocity() const noexcept {
    return velocity_;
}

void Entity::set_velocity(const Velocity& velocity) noexcept {
    velocity_ = velocity;
}

bool Entity::on_ground() const noexcept {
    return on_ground_;
}

void Entity::set_on_ground(bool on_ground) noexcept {
    on_ground_ = on_ground;
}

double Entity::width() const noexcept {
    return width_;
}

double Entity::height_value() const noexcept {
    return height_;
}

Aabb Entity::bounds() const noexcept {
    const double half_width = width_ * 0.5;
    return {
        {position_.x - half_width, position_.y, position_.z - half_width},
        {position_.x + half_width, position_.y + height_, position_.z + half_width},
    };
}

} // namespace mcpi::game
