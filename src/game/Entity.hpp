#pragma once

#include "game/GameApi.hpp"

namespace mcpi::game {

struct Velocity {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    bool operator==(const Velocity&) const = default;
};

struct Aabb {
    Vec3 min{};
    Vec3 max{};

    bool operator==(const Aabb&) const = default;
};

class Entity {
public:
    explicit Entity(int id = 0, double width = 0.6, double height = 1.8) noexcept;
    virtual ~Entity() = default;

    [[nodiscard]] int id() const noexcept;
    [[nodiscard]] const Vec3& position() const noexcept;
    void set_position(const Vec3& position) noexcept;

    [[nodiscard]] const Velocity& velocity() const noexcept;
    void set_velocity(const Velocity& velocity) noexcept;

    [[nodiscard]] bool on_ground() const noexcept;
    void set_on_ground(bool on_ground) noexcept;

    [[nodiscard]] double width() const noexcept;
    [[nodiscard]] double height_value() const noexcept;
    [[nodiscard]] Aabb bounds() const noexcept;

private:
    int id_ = 0;
    Vec3 position_{};
    Velocity velocity_{};
    double width_ = 0.6;
    double height_ = 1.8;
    bool on_ground_ = false;
};

} // namespace mcpi::game
