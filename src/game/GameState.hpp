#pragma once

#include "game/GameApi.hpp"
#include "world/World.hpp"

#include <string>
#include <vector>

namespace mcpi::game {

class GameState final : public GameApi {
public:
    [[nodiscard]] Vec3 player_position() const override;
    void set_player_position(const Vec3& position) override;

    [[nodiscard]] int block_type(int x, int y, int z) const override;
    void set_block(int x, int y, int z, int block_type, int block_data) override;

    void post_chat(const std::string& message) override;

    [[nodiscard]] world::World& world() noexcept;
    [[nodiscard]] const world::World& world() const noexcept;
    [[nodiscard]] const std::vector<std::string>& chat_messages() const noexcept;

private:
    Vec3 player_position_{};
    world::World world_;
    std::vector<std::string> chat_messages_;
};

} // namespace mcpi::game
