#pragma once

#include "game/Entity.hpp"
#include "game/Inventory.hpp"

namespace mcpi::game {

class Player final : public Entity {
public:
    static constexpr double half_width = 0.3;
    static constexpr double height = 1.8;
    static constexpr double jump_velocity = 0.42;

    explicit Player(int id = 0) noexcept;

    [[nodiscard]] Inventory& inventory() noexcept;
    [[nodiscard]] const Inventory& inventory() const noexcept;

    void jump() noexcept;

private:
    Inventory inventory_;
};

} // namespace mcpi::game
