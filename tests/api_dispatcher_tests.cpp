#include "api/ApiDispatcher.hpp"
#include "game/GameApi.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

class FakeGameApi final : public mcpi::game::GameApi {
public:
    mcpi::game::Vec3 player_position() const override {
        return position;
    }

    void set_player_position(const mcpi::game::Vec3& value) override {
        position = value;
        set_position_calls++;
    }

    int block_type(int x, int y, int z) const override {
        last_block_x = x;
        last_block_y = y;
        last_block_z = z;
        return block_type_result;
    }

    void set_block(int x, int y, int z, int block_type, int block_data) override {
        last_block_x = x;
        last_block_y = y;
        last_block_z = z;
        last_block_type = block_type;
        last_block_data = block_data;
        set_block_calls++;
    }

    void post_chat(const std::string& message) override {
        last_chat_message = message;
        chat_calls++;
    }

    mutable int last_block_x = 0;
    mutable int last_block_y = 0;
    mutable int last_block_z = 0;
    int last_block_type = 0;
    int last_block_data = 0;
    int block_type_result = 57;
    int set_position_calls = 0;
    int set_block_calls = 0;
    int chat_calls = 0;
    std::string last_chat_message;
    mcpi::game::Vec3 position{1.5, 2.0, 3.25};
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
        expect(response.has_value(), "player.getPos should return a response");
        expect(*response == "1.5,2,3.25", "player.getPos should use MCPI comma-separated coordinates");
    }

    {
        const auto response = dispatcher.dispatch(command("player.setPos", {"10.5", "20", "-3.25"}));
        expect(!response.has_value(), "player.setPos should be a one-way command");
        expect(game.set_position_calls == 1, "player.setPos should update the game bridge once");
        expect(game.position.x == 10.5 && game.position.y == 20.0 && game.position.z == -3.25,
               "player.setPos should parse floating-point coordinates");
    }

    {
        const auto response = dispatcher.dispatch(command("world.getBlock", {"4", "5", "6"}));
        expect(response.has_value() && *response == "57", "world.getBlock should return the block type id");
        expect(game.last_block_x == 4 && game.last_block_y == 5 && game.last_block_z == 6,
               "world.getBlock should pass integer coordinates to the game bridge");
    }

    {
        dispatcher.dispatch(command("world.setBlock", {"1", "2", "3", "35"}));
        expect(game.set_block_calls == 1, "world.setBlock should update the game bridge");
        expect(game.last_block_type == 35 && game.last_block_data == 0,
               "world.setBlock without data should default block data to zero");

        dispatcher.dispatch(command("world.setBlock", {"1", "2", "3", "35", "14"}));
        expect(game.set_block_calls == 2, "world.setBlock with data should update the game bridge");
        expect(game.last_block_data == 14, "world.setBlock should preserve explicit block data");
    }

    {
        const auto response = dispatcher.dispatch(command("chat.post", {"Hello from Python"}));
        expect(!response.has_value(), "chat.post should be a one-way command");
        expect(game.chat_calls == 1 && game.last_chat_message == "Hello from Python",
               "chat.post should forward the message unchanged");
    }

    {
        const auto response = dispatcher.dispatch(command("player.getPos", {"unexpected"}));
        expect(response.has_value() && *response == "Fail",
               "known request commands with invalid arguments should fail instead of hanging a client");
    }

    {
        const auto response = dispatcher.dispatch(command("world.getBlock", {"x", "2", "3"}));
        expect(response.has_value() && *response == "Fail",
               "invalid numeric request arguments should return Fail");
    }

    {
        const auto response = dispatcher.dispatch(command("not.implemented"));
        expect(!response.has_value(), "unknown commands should remain unhandled until compatibility is measured");
    }

    std::cout << "API dispatcher tests passed.\n";
    return 0;
}
