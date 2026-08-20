#pragma once

#include "game/GameApi.hpp"
#include "game/Inventory.hpp"
#include "world/BlockBehavior.hpp"
#include "world/LightEngine.hpp"
#include "world/World.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mcpi::game {

class GameState final : public GameApi {
public:
    static constexpr int world_min_x = 0;
    static constexpr int world_max_x = 255;
    static constexpr int world_min_y = 0;
    static constexpr int world_max_y = 127;
    static constexpr int world_min_z = 0;
    static constexpr int world_max_z = 255;
    static constexpr int hotbar_size = Inventory::hotbar_size;

    GameState();

    void new_world(std::uint32_t seed);
    [[nodiscard]] bool save(const std::filesystem::path& path) const;
    [[nodiscard]] bool load(const std::filesystem::path& path);
    [[nodiscard]] std::uint32_t seed() const noexcept;
    [[nodiscard]] bool generated_world() const noexcept;

    [[nodiscard]] Vec3 player_position() const override;
    void set_player_position(const Vec3& position) override;
    void move_player(const Vec3& delta) noexcept;
    [[nodiscard]] IVec3 spawn_position() const override;
    void set_spawn_position(const IVec3& position) noexcept;

    [[nodiscard]] int block_type(int x, int y, int z) const override;
    [[nodiscard]] int block_data(int x, int y, int z) const override;
    void set_block(int x, int y, int z, int block_type, int block_data) override;
    void set_blocks(int x1, int y1, int z1,
                    int x2, int y2, int z2,
                    int block_type, int block_data) override;
    [[nodiscard]] int height_at(int x, int z) const override;

    [[nodiscard]] Inventory& inventory() noexcept;
    [[nodiscard]] const Inventory& inventory() const noexcept;
    [[nodiscard]] int selected_hotbar_slot() const noexcept;
    void select_hotbar_slot(int slot) noexcept;
    [[nodiscard]] int hotbar_block(int slot) const noexcept;
    void set_hotbar_block(int slot, int block_type) noexcept;
    [[nodiscard]] int selected_block() const noexcept;
    void place_selected_block(int x, int y, int z);
    void break_block(int x, int y, int z);

    void save_checkpoint() override;
    void restore_checkpoint() override;

    void set_world_setting(const std::string& key, bool value) override;
    void set_player_setting(const std::string& key, bool value) override;
    [[nodiscard]] bool world_setting(const std::string& key) const;
    [[nodiscard]] bool player_setting(const std::string& key) const;

    void set_camera_mode(CameraMode mode) override;
    void set_camera_position(const Vec3& position) override;
    [[nodiscard]] CameraMode camera_mode() const noexcept;
    [[nodiscard]] Vec3 camera_position() const noexcept;

    [[nodiscard]] std::vector<BlockHit> poll_block_hits() override;
    void clear_events() override;
    void add_block_hit(const BlockHit& hit);

    void post_chat(const std::string& message) override;

    [[nodiscard]] world::World& world() noexcept;
    [[nodiscard]] const world::World& world() const noexcept;
    [[nodiscard]] const std::vector<std::string>& chat_messages() const noexcept;

private:
    struct Checkpoint {
        world::World world;
        Vec3 player_position;
        std::unordered_map<std::uint32_t, world::BlockState> changes;
    };

    [[nodiscard]] static bool inside_world(int x, int y, int z) noexcept;
    [[nodiscard]] static std::uint32_t block_key(int x, int y, int z) noexcept;
    [[nodiscard]] static world::BlockPos block_position(std::uint32_t key) noexcept;
    void reset_hotbar() noexcept;

    std::uint32_t seed_ = 0;
    bool generated_world_ = false;
    IVec3 spawn_position_{128, 64, 128};
    Vec3 player_position_{128.0, 64.0, 128.0};
    world::World world_;
    world::LightEngine light_engine_;
    world::BlockUpdateEngine block_updates_;
    std::unordered_map<std::uint32_t, world::BlockState> changes_;
    Inventory inventory_;
    std::optional<Checkpoint> checkpoint_;
    std::unordered_map<std::string, bool> world_settings_;
    std::unordered_map<std::string, bool> player_settings_;
    CameraMode camera_mode_ = CameraMode::Normal;
    Vec3 camera_position_{128.0, 64.0, 128.0};
    std::vector<BlockHit> block_hits_;
    std::vector<std::string> chat_messages_;
};

} // namespace mcpi::game
