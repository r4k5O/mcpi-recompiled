#include "game/GameState.hpp"

#include "world/WorldGenerator.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>

namespace mcpi::game {
namespace {

constexpr const char* save_magic = "MCPI_RECOMPILED_WORLD";
constexpr int save_version = 1;

} // namespace

GameState::GameState() {
    world_settings_.emplace("world_immutable", false);
    world_settings_.emplace("nametags_visible", true);
    player_settings_.emplace("autojump", true);
    reset_hotbar();
}

void GameState::reset_hotbar() noexcept {
    hotbar_ = {1, 3, 4, 5, 20, 45, 46, 57, 89};
    selected_hotbar_slot_ = 0;
}

bool GameState::inside_world(int x, int y, int z) noexcept {
    return x >= world_min_x && x <= world_max_x &&
           y >= world_min_y && y <= world_max_y &&
           z >= world_min_z && z <= world_max_z;
}

std::uint32_t GameState::block_key(int x, int y, int z) noexcept {
    return static_cast<std::uint32_t>(x & 0xff) |
           (static_cast<std::uint32_t>(z & 0xff) << 8U) |
           (static_cast<std::uint32_t>(y & 0x7f) << 16U);
}

world::BlockPos GameState::block_position(std::uint32_t key) noexcept {
    return {
        static_cast<int>(key & 0xffU),
        static_cast<int>((key >> 16U) & 0x7fU),
        static_cast<int>((key >> 8U) & 0xffU),
    };
}

void GameState::new_world(std::uint32_t seed) {
    seed_ = seed;
    generated_world_ = true;
    world::WorldGenerator::generate(world_, seed_);
    light_engine_.rebuild(world_);
    block_updates_.clear();
    changes_.clear();
    checkpoint_.reset();
    reset_hotbar();
    clear_events();
    chat_messages_.clear();

    spawn_position_ = {128, world_.height_at(128, 128), 128};
    player_position_ = {
        static_cast<double>(spawn_position_.x),
        static_cast<double>(spawn_position_.y),
        static_cast<double>(spawn_position_.z),
    };
    camera_position_ = player_position_;
    camera_mode_ = CameraMode::Normal;
}

bool GameState::save(const std::filesystem::path& path) const {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }

    output << save_magic << ' ' << save_version << '\n';
    output << "generated " << (generated_world_ ? 1 : 0) << '\n';
    output << "seed " << seed_ << '\n';
    output << "spawn " << spawn_position_.x << ' ' << spawn_position_.y << ' ' << spawn_position_.z << '\n';
    output << std::setprecision(17);
    output << "player " << player_position_.x << ' ' << player_position_.y << ' ' << player_position_.z << '\n';
    output << "selected " << selected_hotbar_slot_ << '\n';
    output << "hotbar";
    for (int block : hotbar_) {
        output << ' ' << block;
    }
    output << '\n';
    output << "changes " << changes_.size() << '\n';
    for (const auto& [key, block] : changes_) {
        output << key << ' ' << block.id << ' ' << block.data << '\n';
    }

    return static_cast<bool>(output);
}

bool GameState::load(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    std::string magic;
    int version = 0;
    if (!(input >> magic >> version) || magic != save_magic || version != save_version) {
        return false;
    }

    std::string label;
    int generated = 0;
    std::uint32_t loaded_seed = 0;
    IVec3 loaded_spawn{};
    Vec3 loaded_player{};
    int loaded_selected = 0;
    std::array<int, hotbar_size> loaded_hotbar{};
    std::size_t change_count = 0;

    if (!(input >> label >> generated) || label != "generated") return false;
    if (!(input >> label >> loaded_seed) || label != "seed") return false;
    if (!(input >> label >> loaded_spawn.x >> loaded_spawn.y >> loaded_spawn.z) || label != "spawn") return false;
    if (!(input >> label >> loaded_player.x >> loaded_player.y >> loaded_player.z) || label != "player") return false;
    if (!(input >> label >> loaded_selected) || label != "selected") return false;
    if (!(input >> label) || label != "hotbar") return false;
    for (int& block : loaded_hotbar) {
        if (!(input >> block)) return false;
    }
    if (!(input >> label >> change_count) || label != "changes") return false;
    if (change_count > 8'388'608U) {
        return false;
    }

    std::unordered_map<std::uint32_t, world::BlockState> loaded_changes;
    loaded_changes.reserve(change_count);
    for (std::size_t index = 0; index < change_count; ++index) {
        std::uint32_t key = 0;
        world::BlockState block;
        if (!(input >> key >> block.id >> block.data)) {
            return false;
        }
        loaded_changes.insert_or_assign(key, block);
    }

    if (generated != 0) {
        new_world(loaded_seed);
    } else {
        seed_ = loaded_seed;
        generated_world_ = false;
        world_.clear();
        changes_.clear();
        checkpoint_.reset();
        block_updates_.clear();
    }

    spawn_position_ = loaded_spawn;
    player_position_ = loaded_player;
    hotbar_ = loaded_hotbar;
    selected_hotbar_slot_ = std::clamp(loaded_selected, 0, hotbar_size - 1);

    changes_ = loaded_changes;
    for (const auto& [key, block] : changes_) {
        const auto position = block_position(key);
        world_.set_block(position, block);
    }
    light_engine_.rebuild(world_);
    block_updates_.clear();

    camera_position_ = player_position_;
    return true;
}

std::uint32_t GameState::seed() const noexcept {
    return seed_;
}

bool GameState::generated_world() const noexcept {
    return generated_world_;
}

Vec3 GameState::player_position() const {
    return player_position_;
}

void GameState::set_player_position(const Vec3& position) {
    player_position_ = position;
}

void GameState::move_player(const Vec3& delta) noexcept {
    constexpr double max_xz = 255.999;
    constexpr double max_y = 127.999;
    player_position_.x = std::clamp(player_position_.x + delta.x, 0.0, max_xz);
    player_position_.y = std::clamp(player_position_.y + delta.y, 0.0, max_y);
    player_position_.z = std::clamp(player_position_.z + delta.z, 0.0, max_xz);
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

    const world::BlockPos position{x, y, z};
    const world::BlockState before = world_.block_at(position);
    const world::BlockState after{block_type, block_data};

    world_.set_block(position, after);
    changes_.insert_or_assign(block_key(x, y, z), after);
    block_updates_.on_block_changed(world_, position, before, after);

    if (before != after) {
        light_engine_.on_block_changed(world_, position, before, after);
    }
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

    const world::BlockState after{block_type, block_data};
    bool changed = false;

    for (int x = min_x; x <= max_x; ++x) {
        for (int z = min_z; z <= max_z; ++z) {
            for (int y = min_y; y <= max_y; ++y) {
                const world::BlockPos position{x, y, z};
                const world::BlockState before = world_.block_at(position);

                world_.set_block(position, after);
                changes_.insert_or_assign(block_key(x, y, z), after);
                block_updates_.on_block_changed(world_, position, before, after);
                changed = changed || before != after;
            }
        }
    }

    if (changed) {
        light_engine_.rebuild(world_);
    }
}

int GameState::height_at(int x, int z) const {
    if (x < world_min_x || x > world_max_x || z < world_min_z || z > world_max_z) {
        return 0;
    }
    return world_.height_at(x, z);
}

int GameState::selected_hotbar_slot() const noexcept {
    return selected_hotbar_slot_;
}

void GameState::select_hotbar_slot(int slot) noexcept {
    if (slot >= 0 && slot < hotbar_size) {
        selected_hotbar_slot_ = slot;
    }
}

int GameState::hotbar_block(int slot) const noexcept {
    return slot >= 0 && slot < hotbar_size ? hotbar_[static_cast<std::size_t>(slot)] : 0;
}

void GameState::set_hotbar_block(int slot, int block_type) noexcept {
    if (slot >= 0 && slot < hotbar_size) {
        hotbar_[static_cast<std::size_t>(slot)] = block_type;
    }
}

int GameState::selected_block() const noexcept {
    return hotbar_block(selected_hotbar_slot_);
}

void GameState::place_selected_block(int x, int y, int z) {
    if (world_setting("world_immutable")) {
        return;
    }
    set_block(x, y, z, selected_block(), 0);
}

void GameState::break_block(int x, int y, int z) {
    if (world_setting("world_immutable")) {
        return;
    }
    set_block(x, y, z, 0, 0);
}

void GameState::save_checkpoint() {
    checkpoint_ = Checkpoint{world_, player_position_, changes_};
}

void GameState::restore_checkpoint() {
    if (!checkpoint_.has_value()) {
        return;
    }
    world_ = checkpoint_->world;
    player_position_ = checkpoint_->player_position;
    changes_ = checkpoint_->changes;
    block_updates_.clear();
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
