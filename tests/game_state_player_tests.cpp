#include "game/GameState.hpp"
#include "game/Player.hpp"

#include <cassert>
#include <cmath>

namespace {

bool close(double a, double b, double epsilon = 1.0e-6) {
    return std::abs(a - b) <= epsilon;
}

} // namespace

int main() {
    using mcpi::game::GameState;
    using mcpi::game::Vec3;

    GameState game;
    game.new_world(0x10203040U);

    // GameState must expose the actual local player, not a mirrored position.
    assert(game.player().id() == 0);
    assert(game.player().position() == game.player_position());
    assert(&game.player().inventory() == &game.inventory());

    const Vec3 teleported{2.5, 4.0, 2.5};
    game.set_player_position(teleported);
    assert(game.player().position() == teleported);

    const Vec3 direct_entity_update{3.5, 5.0, 3.5};
    game.player().set_position(direct_entity_update);
    assert(game.player_position() == direct_entity_update);

    // Client movement must go through the same collision layer as Physics.
    game.world().clear();
    for (int x = 0; x <= 10; ++x) {
        for (int z = 0; z <= 10; ++z) {
            game.set_block(x, 0, z, 1, 0);
        }
    }
    game.set_block(4, 1, 2, 1, 0);
    game.set_block(4, 2, 2, 1, 0);
    game.set_player_position({2.5, 1.0, 2.5});
    game.move_player({5.0, 0.0, 0.0});
    assert(close(game.player_position().x, 3.7));
    assert(game.player().position() == game.player_position());

    // Hotbar accessors are compatibility views over Player::inventory().
    game.select_hotbar_slot(3);
    game.set_hotbar_block(3, 57);
    assert(game.player().inventory().selected_slot() == 3);
    assert(game.player().inventory().slot(3).item_id == 57);
    assert(game.selected_block() == 57);

    // Checkpoint restore must restore the authoritative player position.
    game.save_checkpoint();
    game.set_player_position({8.0, 8.0, 8.0});
    game.restore_checkpoint();
    assert(close(game.player().position().x, 3.7));
    assert(close(game.player().position().y, 1.0));
    assert(close(game.player().position().z, 2.5));

    return 0;
}
