#include "game/GameState.hpp"

#include <cstdlib>
#include <filesystem>
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
    using mcpi::game::GameState;
    using mcpi::game::Vec3;

    GameState first;
    first.new_world(123456789U);

    expect(first.seed() == 123456789U, "new_world should retain its seed");
    const auto spawn = first.spawn_position();
    expect(spawn.x == 128 && spawn.z == 128,
           "Phase 1 finite world should spawn near the world center");
    expect(spawn.y == first.height_at(spawn.x, spawn.z),
           "spawn Y should be the first air block above terrain");
    expect(first.block_type(spawn.x, spawn.y - 1, spawn.z) != 0,
           "spawn should have terrain directly below it");

    GameState same;
    same.new_world(123456789U);
    expect(first.height_at(10, 10) == same.height_at(10, 10) &&
               first.height_at(128, 128) == same.height_at(128, 128) &&
               first.height_at(245, 200) == same.height_at(245, 200),
           "the same seed should generate the same terrain");

    GameState different;
    different.new_world(987654321U);
    const bool differs =
        first.height_at(10, 10) != different.height_at(10, 10) ||
        first.height_at(128, 128) != different.height_at(128, 128) ||
        first.height_at(245, 200) != different.height_at(245, 200);
    expect(differs, "different seeds should influence Phase 1 terrain");

    expect(first.selected_hotbar_slot() == 0,
           "new worlds should begin on the first hotbar slot");
    expect(first.hotbar_block(0) != 0,
           "the Phase 1 hotbar should contain placeable blocks");

    first.select_hotbar_slot(7);
    first.set_hotbar_block(7, 57);
    expect(first.selected_hotbar_slot() == 7 && first.selected_block() == 57,
           "hotbar selection should choose the block used for placement");

    const int place_x = spawn.x + 2;
    const int place_y = spawn.y + 1;
    const int place_z = spawn.z;
    first.place_selected_block(place_x, place_y, place_z);
    expect(first.block_type(place_x, place_y, place_z) == 57,
           "gameplay placement should modify the same World used by the API");

    first.break_block(place_x, place_y, place_z);
    expect(first.block_type(place_x, place_y, place_z) == 0,
           "gameplay breaking should replace a block with air");

    first.set_world_setting("world_immutable", true);
    first.place_selected_block(place_x, place_y, place_z);
    expect(first.block_type(place_x, place_y, place_z) == 0,
           "world_immutable should block direct player placement");
    first.set_world_setting("world_immutable", false);
    first.place_selected_block(place_x, place_y, place_z);

    first.set_player_position({254.5, 126.0, 1.0});
    first.move_player({10.0, 10.0, -10.0});
    const auto bounded = first.player_position();
    expect(bounded.x < 256.0 && bounded.x >= 0.0 &&
               bounded.y < 128.0 && bounded.y >= 0.0 &&
               bounded.z < 256.0 && bounded.z >= 0.0,
           "player movement should remain inside the finite Pi world");

    const auto save_path = std::filesystem::temp_directory_path() / "mcpi-phase1-world-test.mcpiworld";
    first.set_player_position({130.25, 70.0, 127.75});
    expect(first.save(save_path), "Phase 1 world save should succeed");

    GameState loaded;
    expect(loaded.load(save_path), "Phase 1 world load should succeed");
    expect(loaded.seed() == first.seed(), "save/load should preserve the seed");
    expect(loaded.spawn_position() == first.spawn_position(), "save/load should preserve spawn");
    expect(loaded.player_position() == first.player_position(), "save/load should preserve player position");
    expect(loaded.selected_hotbar_slot() == 7 && loaded.hotbar_block(7) == 57,
           "save/load should preserve hotbar state");
    expect(loaded.block_type(place_x, place_y, place_z) == 57,
           "save/load should preserve placed blocks over regenerated terrain");

    loaded.break_block(spawn.x, spawn.y - 1, spawn.z);
    expect(loaded.block_type(spawn.x, spawn.y - 1, spawn.z) == 0,
           "breaking generated terrain should create an air override");
    expect(loaded.save(save_path), "saving an air override should succeed");

    GameState loaded_again;
    expect(loaded_again.load(save_path), "reloading an air override should succeed");
    expect(loaded_again.block_type(spawn.x, spawn.y - 1, spawn.z) == 0,
           "air overrides must survive regeneration during load");

    std::error_code ignored;
    std::filesystem::remove(save_path, ignored);

    std::cout << "World lifecycle and gameplay tests passed.\n";
    return 0;
}
