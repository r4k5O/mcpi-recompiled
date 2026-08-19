#include "game/GameState.hpp"

#include <algorithm>

namespace mcpi::game {

GameState::GameState() {
    world_settings_.emplace("world_immutable", false);
    world_settings_.emplace("nametags_visible", true);
    player_settings_.emplace("autojump", true);
}

bool GameState::inside_world(int x, int y, int z) noexcept {
    return x >= world_min_x && x <= world_max_x &&
           y >= world_min_y && y <= world_max_y &&
           z >= world_min_z && z <= world_max_z;
}

Vec3 GameState::player_position() const {
    return player_position_;
}

void GameState::set_player_position(const Vec3& position) {
    player_position_ = position;
}

IVec3 GameState::spawn_position() const {
    return spawn_position_;
}

void GameState::set_spawn_position(const IVec3& position) noexcept {
    spawn_position_ = position;
}

int GameState::block_type(int x, int y, int z) const {
    return world_.block_at({x, y, z}).id;
}

int GameState::block_data(int x, int y, int z) const {
    return world_.block_at({x, y, z}).data;
}

void GameState::set_block(int x, int y, int z, int block_type, int block_data) {
    if (!inside_world(x, y, z)) {
        return;
    }
    world_.set_block({x, y, z}, {block_type, block_data});
}

void GameState::set_blocks(int x1, int y1, int z1,
                           int x2, int y2, int z2,
                           int block_type, int block_data) {
    const int min_x = std::max(world_min_x, std::min(x1, x2));
    const int max_x = std::min(world_max_x, std::max(x1, x2));
    const int min_y = std::max(world_min_y, std::min(y1, y2));
    const int max_y = std::min(world_max_y, std::max(y1, y2));
    const int min_z = std::max(world_min_z, std::min(z1, z2));
    const int max_z = std::min(world_max_z, std::max(z1, z2));

    if (min_x > max_x || min_y > max_y || min_z > max_z) {
        return;
    }

    for (int x = min_x; x <= max_x; ++x) {
        for (int z = min_z; z <= max_z; ++z) {
            for (int y = min_y; y <= max_y; ++y) {
                world_.set_block({x, y, z}, {block_type, block_data});
            }
        }
    }
}

int GameState::height_at(int x, int z) const {
    if (x < world_min_x || x > world_max_x || z < world_min_z || z > world_max_z) {
        return 0;
    }
    return world_.height_at(x, z);
}

void GameState::save_checkpoint() {
    checkpoint_ = Checkpoint{world_, player_position_};
}

void GameState::restore_checkpoint() {
    if (!checkpoint_.has_value()) {
        return;
    }
    world_ = checkpoint_->world;
    player_position_ = checkpoint_->player_position;
}

void GameState::set_world_setting(const std::string& key, bool value) {
    world_settings_[key] = value;
}

void GameState::set_player_setting(const std::string& key, bool value) {
    player_settings_[key] = value;
}

bool GameState::world_setting(const std::string& key) const {
    const auto found = world_settings_.find(key);
    return found != world_settings_.end() ? found->second : false;
}

bool GameState::player_setting(const std::string& key) const {
    const auto found = player_settings_.find(key);
    return found != player_settings_.end() ? found->second : false;
}

void GameState::set_camera_mode(CameraMode mode) {
    camera_mode_ = mode;
}

void GameState::set_camera_position(const Vec3& position) {
    camera_position_ = position;
}

CameraMode GameState::camera_mode() const noexcept {
    return camera_mode_;
}

Vec3 GameState::camera_position() const noexcept {
    return camera_position_;
}

std::vector<BlockHit> GameState::poll_block_hits() {
    auto result = block_hits_;
    block_hits_.clear();
    return result;
}

void GameState::clear_events() {
    block_hits_.clear();
}

void GameState::add_block_hit(const BlockHit& hit) {
    block_hits_.push_back(hit);
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
