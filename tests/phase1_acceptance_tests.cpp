#include "api/ApiDispatcher.hpp"
#include "game/GameState.hpp"
#include "world/Chunk.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "PHASE 1 ACCEPTANCE FAILED: " << message << '\n';
        std::exit(1);
    }
}

mcpi::api::Command command(std::string name, std::initializer_list<std::string> arguments = {}) {
    return {std::move(name), arguments};
}

} // namespace

int main() {
    using mcpi::game::GameState;
    using mcpi::world::Chunk;

    require(Chunk::width == 16 && Chunk::height == 128 && Chunk::depth == 16,
            "LevelChunk dimensions must remain 16x128x16");
    require(Chunk::block_count == 32768 && Chunk::nibble_storage_size == 16384,
            "LevelChunk block and nibble storage sizes must match the original layout evidence");

    GameState game;
    game.new_world(0x504931U);
    require(game.generated_world(), "new_world must create a playable finite world");

    const auto spawn = game.spawn_position();
    require(spawn.x == 128 && spawn.z == 128,
            "finite Phase 1 world must place spawn at its horizontal center");
    require(game.block_type(spawn.x, spawn.y - 1, spawn.z) != 0,
            "spawn must be above solid generated terrain");

    mcpi::api::ApiDispatcher api(game);

    const auto initial_pos = api.dispatch(command("player.getPos"));
    require(initial_pos.has_value() && *initial_pos == "0,0,0",
            "MCPI player coordinates must be spawn-relative at world creation");

    api.dispatch(command("player.setPos", {"2.5", "3", "-1.25"}));
    const auto moved = game.player_position();
    require(moved.x == spawn.x + 2.5 && moved.y == spawn.y + 3.0 && moved.z == spawn.z - 1.25,
            "spawn-relative API coordinates must translate into internal world coordinates");

    api.dispatch(command("world.setBlock", {"3", "2", "4", "57", "3"}));
    const auto block = api.dispatch(command("world.getBlockWithData", {"3", "2", "4"}));
    require(block.has_value() && *block == "57,3",
            "API block id and metadata must round-trip through LevelChunk storage");

    api.dispatch(command("world.setBlocks", {"-1", "1", "-1", "1", "2", "1", "41"}));
    require(api.dispatch(command("world.getBlock", {"-1", "1", "-1"})) == std::optional<std::string>("41") &&
                api.dispatch(command("world.getBlock", {"1", "2", "1"})) == std::optional<std::string>("41"),
            "world.setBlocks must fill both inclusive corners");

    api.dispatch(command("world.checkpoint.save"));
    api.dispatch(command("world.setBlock", {"3", "2", "4", "0"}));
    require(api.dispatch(command("world.getBlock", {"3", "2", "4"})) == std::optional<std::string>("0"),
            "checkpoint acceptance setup must modify the world");
    api.dispatch(command("world.checkpoint.restore"));
    require(api.dispatch(command("world.getBlockWithData", {"3", "2", "4"})) == std::optional<std::string>("57,3"),
            "checkpoint restore must restore world and metadata state");

    game.select_hotbar_slot(7);
    game.set_hotbar_block(7, 45);
    const int direct_x = spawn.x + 5;
    const int direct_y = spawn.y + 2;
    const int direct_z = spawn.z;
    game.place_selected_block(direct_x, direct_y, direct_z);
    require(game.block_type(direct_x, direct_y, direct_z) == 45,
            "direct gameplay placement must modify the same world as the API");

    game.add_block_hit({{direct_x, direct_y, direct_z}, 1, 0});
    const auto hits = api.dispatch(command("events.block.hits"));
    require(hits.has_value() && hits->find("5,2,0,1,0") != std::string::npos,
            "block-hit events must be exposed in spawn-relative coordinates");

    const auto save_path = std::filesystem::temp_directory_path() / "mcpi-phase1-acceptance.mcpiworld";
    require(game.save(save_path), "world save must succeed");

    GameState loaded;
    require(loaded.load(save_path), "world load must succeed");
    require(loaded.seed() == game.seed(), "world seed must persist");
    require(loaded.spawn_position() == game.spawn_position(), "spawn must persist");
    require(loaded.selected_hotbar_slot() == 7 && loaded.hotbar_block(7) == 45,
            "hotbar state must persist");
    require(loaded.block_type(direct_x, direct_y, direct_z) == 45,
            "gameplay block changes must persist over deterministic regeneration");
    require(loaded.block_type(spawn.x + 3, spawn.y + 2, spawn.z + 4) == 57 &&
                loaded.block_data(spawn.x + 3, spawn.y + 2, spawn.z + 4) == 3,
            "API block metadata changes must persist");

    std::error_code ignored;
    std::filesystem::remove(save_path, ignored);

    std::cout << "Phase 1 acceptance passed.\n";
    return 0;
}
