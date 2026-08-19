#include "api/ApiDispatcher.hpp"
#include "game/GameApi.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

class FakeGameApi final : public mcpi::game::GameApi {
public:
    mcpi::game::Vec3 player_position() const override { return position; }
    void set_player_position(const mcpi::game::Vec3& value) override {
        position = value;
        ++set_position_calls;
    }

    mcpi::game::IVec3 spawn_position() const override { return spawn; }

    int block_type(int x, int y, int z) const override {
        record_block_position(x, y, z);
        return block_type_result;
    }
    int block_data(int x, int y, int z) const override {
        record_block_position(x, y, z);
        return block_data_result;
    }
    void set_block(int x, int y, int z, int block_type, int block_data) override {
        record_block_position(x, y, z);
        last_block_type = block_type;
        last_block_data = block_data;
        ++set_block_calls;
    }
    void set_blocks(int x1, int y1, int z1,
                    int x2, int y2, int z2,
                    int block_type, int block_data) override {
        range = {x1, y1, z1, x2, y2, z2};
        last_block_type = block_type;
        last_block_data = block_data;
        ++set_blocks_calls;
    }
    int height_at(int x, int z) const override {
        last_height_x = x;
        last_height_z = z;
        return height_result;
    }

    void save_checkpoint() override { ++checkpoint_save_calls; }
    void restore_checkpoint() override { ++checkpoint_restore_calls; }

    void set_world_setting(const std::string& key, bool value) override {
        last_setting_key = key;
        last_setting_value = value;
        ++world_setting_calls;
    }
    void set_player_setting(const std::string& key, bool value) override {
        last_setting_key = key;
        last_setting_value = value;
        ++player_setting_calls;
    }

    void set_camera_mode(mcpi::game::CameraMode mode) override {
        camera_mode = mode;
        ++camera_mode_calls;
    }
    void set_camera_position(const mcpi::game::Vec3& value) override {
        camera_position = value;
        ++camera_position_calls;
    }

    std::vector<mcpi::game::BlockHit> poll_block_hits() override {
        ++poll_hits_calls;
        return hits;
    }
    void clear_events() override { ++clear_events_calls; }

    void post_chat(const std::string& message) override {
        last_chat_message = message;
        ++chat_calls;
    }

    void record_block_position(int x, int y, int z) const {
        last_block_x = x;
        last_block_y = y;
        last_block_z = z;
    }

    mcpi::game::IVec3 spawn{100, 64, 200};
    mcpi::game::Vec3 position{101.5, 66.0, 203.25};
    mcpi::game::CameraMode camera_mode = mcpi::game::CameraMode::Normal;
    mcpi::game::Vec3 camera_position{};
    std::vector<mcpi::game::BlockHit> hits{{{101, 65, 202}, 3, 0}};

    mutable int last_block_x = 0;
    mutable int last_block_y = 0;
    mutable int last_block_z = 0;
    mutable int last_height_x = 0;
    mutable int last_height_z = 0;
    std::vector<int> range;
    int last_block_type = 0;
    int last_block_data = 0;
    int block_type_result = 57;
    int block_data_result = 14;
    int height_result = 72;
    int set_position_calls = 0;
    int set_block_calls = 0;
    int set_blocks_calls = 0;
    int checkpoint_save_calls = 0;
    int checkpoint_restore_calls = 0;
    int world_setting_calls = 0;
    int player_setting_calls = 0;
    int camera_mode_calls = 0;
    int camera_position_calls = 0;
    int poll_hits_calls = 0;
    int clear_events_calls = 0;
    int chat_calls = 0;
    std::string last_setting_key;
    bool last_setting_value = false;
    std::string last_chat_message;
};

mcpi::api::Command command(std::string name, std::initializer_list<std::string> args = {}) {
    return mcpi::api::Command{std::move(name), args};
}

} // namespace

int main() {
    FakeGameApi game;
    mcpi::api::ApiDispatcher dispatcher(game);

    {
        const auto response = dispatcher.dispatch(command("player.getPos"));
        expect(response.has_value() && *response == "1.5,2,3.25",
               "player.getPos should return spawn-relative exact coordinates");
    }

    {
        dispatcher.dispatch(command("player.setPos", {"10.5", "20", "-3.25"}));
        expect(game.set_position_calls == 1, "player.setPos should update the game bridge");
        expect(game.position.x == 110.5 && game.position.y == 84.0 && game.position.z == 196.75,
               "player.setPos should translate API coordinates by SpawnX/Y/Z");
    }

    {
        game.position = {101.9, 66.1, 203.8};
        const auto response = dispatcher.dispatch(command("player.getTile"));
        expect(response.has_value() && *response == "1,2,3",
               "player.getTile should floor and translate the player position");

        dispatcher.dispatch(command("player.setTile", {"-2", "5", "7"}));
        expect(game.position.x == 98.0 && game.position.y == 69.0 && game.position.z == 207.0,
               "player.setTile should translate integer API coordinates to internal coordinates");
    }

    {
        const auto response = dispatcher.dispatch(command("world.getBlock", {"4", "5", "6"}));
        expect(response.has_value() && *response == "57", "world.getBlock should return the block type id");
        expect(game.last_block_x == 104 && game.last_block_y == 69 && game.last_block_z == 206,
               "world.getBlock should translate spawn-relative coordinates");

        const auto with_data = dispatcher.dispatch(command("world.getBlockWithData", {"4", "5", "6"}));
        expect(with_data.has_value() && *with_data == "57,14",
               "world.getBlockWithData should return id and data");
    }

    {
        dispatcher.dispatch(command("world.setBlock", {"1", "2", "3", "35", "14"}));
        expect(game.last_block_x == 101 && game.last_block_y == 66 && game.last_block_z == 203,
               "world.setBlock should use internal spawn-shifted coordinates");
        expect(game.last_block_type == 35 && game.last_block_data == 14,
               "world.setBlock should preserve id and data");

        dispatcher.dispatch(command("world.setBlocks", {"-1", "-2", "-3", "2", "3", "4", "41", "7"}));
        expect(game.set_blocks_calls == 1,
               "world.setBlocks should call the bulk game bridge");
        expect(game.range == std::vector<int>({99, 62, 197, 102, 67, 204}),
               "world.setBlocks should translate both endpoints");
        expect(game.last_block_type == 41 && game.last_block_data == 7,
               "world.setBlocks should preserve block id/data");
    }

    {
        const auto response = dispatcher.dispatch(command("world.getHeight", {"4", "6"}));
        expect(response.has_value() && *response == "8",
               "world.getHeight should translate the internal height back relative to SpawnY");
        expect(game.last_height_x == 104 && game.last_height_z == 206,
               "world.getHeight should translate X/Z by spawn");
    }

    {
        dispatcher.dispatch(command("world.checkpoint.save"));
        dispatcher.dispatch(command("world.checkpoint.restore"));
        expect(game.checkpoint_save_calls == 1 && game.checkpoint_restore_calls == 1,
               "checkpoint commands should reach GameApi");

        dispatcher.dispatch(command("world.setting", {"world_immutable", "1"}));
        expect(game.world_setting_calls == 1 && game.last_setting_key == "world_immutable" && game.last_setting_value,
               "world.setting should forward boolean settings");

        dispatcher.dispatch(command("player.setting", {"autojump", "0"}));
        expect(game.player_setting_calls == 1 && game.last_setting_key == "autojump" && !game.last_setting_value,
               "player.setting should support the distributed client API");
    }

    {
        dispatcher.dispatch(command("camera.mode.setNormal"));
        expect(game.camera_mode == mcpi::game::CameraMode::Normal,
               "camera normal mode should be supported");

        dispatcher.dispatch(command("camera.mode.setThirdPerson"));
        expect(game.camera_mode == mcpi::game::CameraMode::ThirdPerson,
               "protocol third-person camera command should be supported");

        dispatcher.dispatch(command("camera.mode.setFollow"));
        expect(game.camera_mode == mcpi::game::CameraMode::ThirdPerson,
               "distributed client setFollow alias should be supported");

        dispatcher.dispatch(command("camera.mode.setFixed"));
        expect(game.camera_mode == mcpi::game::CameraMode::Fixed,
               "fixed camera mode should be supported");

        dispatcher.dispatch(command("camera.mode.setPos", {"1.5", "2", "3.25"}));
        expect(game.camera_position.x == 101.5 && game.camera_position.y == 66.0 && game.camera_position.z == 203.25,
               "protocol camera position should be spawn-relative");

        dispatcher.dispatch(command("camera.setPos", {"2", "3", "4"}));
        expect(game.camera_position.x == 102.0 && game.camera_position.y == 67.0 && game.camera_position.z == 204.0,
               "distributed client camera.setPos alias should be supported");
    }

    {
        const auto response = dispatcher.dispatch(command("events.block.hits"));
        expect(response.has_value() && *response == "1,1,2,3,0",
               "block-hit events should be serialized with spawn-relative positions");
        expect(game.poll_hits_calls == 1, "events.block.hits should poll GameApi once");

        dispatcher.dispatch(command("events.clear"));
        expect(game.clear_events_calls == 1, "events.clear should clear queued events");
    }

    {
        dispatcher.dispatch(command("chat.post", {"Hello from Python and Java"}));
        expect(game.chat_calls == 1 && game.last_chat_message == "Hello from Python and Java",
               "chat.post should forward the message unchanged");
    }

    {
        const auto response = dispatcher.dispatch(command("world.getBlock", {"x", "2", "3"}));
        expect(response.has_value() && *response == "Fail",
               "invalid numeric request arguments should return Fail");

        const auto unknown = dispatcher.dispatch(command("not.implemented"));
        expect(!unknown.has_value(), "unknown commands should remain one-way/unhandled");
    }

    std::cout << "API dispatcher tests passed.\n";
    return 0;
}
