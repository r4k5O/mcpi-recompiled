#include "game/GameState.hpp"
#include "game/Inventory.hpp"
#include "storage/StorageRouter.hpp"

#include <cassert>
#include <filesystem>
#include <stdexcept>

int main() {
    using mcpi::game::GameState;
    using mcpi::game::Inventory;
    using mcpi::game::ItemStack;

    Inventory inventory;
    assert(inventory.selected_slot() == 0);
    inventory.select(99);
    assert(inventory.selected_slot() == Inventory::hotbar_size - 1);
    inventory.select(-4);
    assert(inventory.selected_slot() == 0);

    inventory.slot(2) = ItemStack{57, 4, 6};
    assert(inventory.slot(2).item_id == 57);
    assert(inventory.slot(2).count == 4);
    assert(inventory.slot(2).data == 6);

    bool threw = false;
    try {
        (void)inventory.slot(Inventory::hotbar_size);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    assert(threw);

    GameState game;
    game.new_world(1234U);
    game.inventory().slot(3) = ItemStack{57, 12, 6};
    game.select_hotbar_slot(3);
    game.place_selected_block(20, 100, 20);
    assert(game.block_type(20, 100, 20) == 57);
    assert(game.block_data(20, 100, 20) == 6);

    // Compatibility accessors continue to expose the selected block id.
    assert(game.hotbar_block(3) == 57);
    game.set_hotbar_block(3, 41);
    assert(game.inventory().slot(3).item_id == 41);
    assert(game.inventory().slot(3).count == 1);
    assert(game.inventory().slot(3).data == 0);

    // The project-owned legacy migration format must preserve full stacks
    // while remaining readable through the common storage router.
    const auto path = std::filesystem::temp_directory_path() /
                      "mcpi-recompiled-inventory.mcpiworld";
    std::filesystem::remove(path);
    game.inventory().slot(4) = ItemStack{45, 23, 2};
    game.select_hotbar_slot(4);
    assert(mcpi::storage::save_world(game, path));

    GameState loaded;
    assert(mcpi::storage::load_world(loaded, path));
    assert(loaded.inventory().slot(4) == ItemStack{45, 23, 2});
    assert(loaded.selected_hotbar_slot() == 4);
    std::filesystem::remove(path);

    return 0;
}
