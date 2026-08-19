#include "game/GameState.hpp"

namespace mcpi::game {

Vec3 GameState::player_position() const {
    return player_position_;
}

void GameState::set_player_position(const Vec3& position) {
    player_position_ = position;
}

int GameState::block_type(int x, int y, int z) const {
    return world_.block_at({x, y, z}).id;
}

void GameState::set_block(int x, int y, int z, int block_type, int block_data) {
    world_.set_block({x, y, z}, {block_type, block_data});
}

void GameState::post_chat(const std::string& message) {
    chat_messages_.push_back(message);
}

world::World& GameState::world() noexcept {
    return world_;
}

const world::World& GameState::world() const noexcept {
    return world_;
}

const std::vector<std::string>& GameState::chat_messages() const noexcept {
    return chat_messages_;
}

} // namespace mcpi::game
