#include "game/GameState.hpp"
#include "world/World.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    {
        mcpi::world::World world;

        const auto air = world.block_at({12, 34, -56});
        expect(air.id == 0 && air.data == 0,
               "unwritten world positions should be air");

        world.set_block({1, 2, 3}, {57, 0});
        const auto diamond = world.block_at({1, 2, 3});
        expect(diamond.id == 57 && diamond.data == 0,
               "world should store a block id at an exact coordinate");

        world.set_block({-8, 64, 19}, {35, 14});
        const auto red_wool = world.block_at({-8, 64, 19});
        expect(red_wool.id == 35 && red_wool.data == 14,
               "world should preserve block data and negative coordinates");

        world.set_block({1, 2, 3}, {1, 0});
        const auto overwritten = world.block_at({1, 2, 3});
        expect(overwritten.id == 1 && overwritten.data == 0,
               "setting a coordinate twice should replace its block state");
    }

    {
        mcpi::game::GameState game;

        game.set_player_position({10.5, 20.0, -3.25});
        const auto position = game.player_position();
        expect(position.x == 10.5 && position.y == 20.0 && position.z == -3.25,
               "GameState should retain the API-visible player position");

        game.set_block(4, 5, 6, 35, 14);
        expect(game.block_type(4, 5, 6) == 35,
               "GameState block_type should read from its reconstructed world");
        const auto state = game.world().block_at({4, 5, 6});
        expect(state.id == 35 && state.data == 14,
               "GameState should pass block id and data into the world");

        game.post_chat("Hello reconstructed world");
        expect(game.chat_messages().size() == 1 &&
                   game.chat_messages().front() == "Hello reconstructed world",
               "GameState should retain chat messages for the future game UI");
    }

    std::cout << "World backend tests passed.\n";
    return 0;
}
