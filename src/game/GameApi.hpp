#pragma once

#include <string>
#include <vector>

namespace mcpi::game {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    bool operator==(const Vec3&) const = default;
};

struct IVec3 {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const IVec3&) const = default;
};

enum class CameraMode {
    Normal,
    ThirdPerson,
    Fixed,
};

struct BlockHit {
    IVec3 position{};
    int face = 0;
    int entity_id = 0;
};

class GameApi {
public:
    virtual ~GameApi() = default;

    [[nodiscard]] virtual Vec3 player_position() const = 0;
    virtual void set_player_position(const Vec3& position) = 0;
    [[nodiscard]] virtual IVec3 spawn_position() const = 0;

    [[nodiscard]] virtual int block_type(int x, int y, int z) const = 0;
    [[nodiscard]] virtual int block_data(int x, int y, int z) const = 0;
    virtual void set_block(int x, int y, int z, int block_type, int block_data) = 0;
    virtual void set_blocks(int x1, int y1, int z1,
                            int x2, int y2, int z2,
                            int block_type, int block_data) = 0;
    [[nodiscard]] virtual int height_at(int x, int z) const = 0;

    virtual void save_checkpoint() = 0;
    virtual void restore_checkpoint() = 0;

    virtual void set_world_setting(const std::string& key, bool value) = 0;
    virtual void set_player_setting(const std::string& key, bool value) = 0;

    virtual void set_camera_mode(CameraMode mode) = 0;
    virtual void set_camera_position(const Vec3& position) = 0;

    [[nodiscard]] virtual std::vector<BlockHit> poll_block_hits() = 0;
    virtual void clear_events() = 0;

    virtual void post_chat(const std::string& message) = 0;
};

} // namespace mcpi::game
