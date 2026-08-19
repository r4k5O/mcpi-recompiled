#pragma once

#include <string>

namespace mcpi::game {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

class GameApi {
public:
    virtual ~GameApi() = default;

    [[nodiscard]] virtual Vec3 player_position() const = 0;
    virtual void set_player_position(const Vec3& position) = 0;

    [[nodiscard]] virtual int block_type(int x, int y, int z) const = 0;
    virtual void set_block(int x, int y, int z, int block_type, int block_data) = 0;

    virtual void post_chat(const std::string& message) = 0;
};

} // namespace mcpi::game
