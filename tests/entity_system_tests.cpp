#include "api/ApiDispatcher.hpp"
#include "api/Command.hpp"
#include "game/EntityRegistry.hpp"
#include "game/GameState.hpp"
#include "game/Player.hpp"

#include <cassert>
#include <memory>
#include <vector>

int main() {
    using mcpi::api::ApiDispatcher;
    using mcpi::api::Command;
    using mcpi::game::Entity;
    using mcpi::game::EntityRegistry;
    using mcpi::game::GameState;
    using mcpi::game::Player;
    using mcpi::game::Vec3;

    Player local_player(0);
    EntityRegistry registry;
    assert(registry.register_external(local_player));
    assert(registry.find(0) == &local_player);
    assert(registry.find(999) == nullptr);

    auto extra = std::make_unique<Entity>(7);
    Entity* extra_ptr = extra.get();
    assert(registry.add(std::move(extra)) == 7);
    assert(registry.find(7) == extra_ptr);
    assert((registry.ids() == std::vector<int>{0, 7}));

    // IDs remain stable and duplicates are rejected rather than replacing.
    assert(registry.add(std::make_unique<Entity>(7)) == -1);
    assert(registry.find(7) == extra_ptr);
    assert(!registry.register_external(local_player));

    GameState game;
    game.new_world(123U);
    assert(game.entities().find(0) == &game.player());
    assert((game.entities().ids() == std::vector<int>{0}));

    ApiDispatcher dispatcher(game);
    const auto player_ids = dispatcher.dispatch(Command{"world.getPlayerIds", {}});
    assert(player_ids.has_value());
    assert(*player_ids == "0");

    game.set_player_position({130.0, 70.0, 130.0});
    const Vec3 before = game.player_position();

    // A missing entity must not silently alias to local player 0.
    const auto missing_get = dispatcher.dispatch(Command{"entity.getPos", {"999"}});
    assert(missing_get.has_value());
    assert(*missing_get == "Fail");

    const auto missing_set = dispatcher.dispatch(
        Command{"entity.setPos", {"999", "10", "20", "30"}});
    assert(!missing_set.has_value());
    assert(game.player_position() == before);

    // Entity 0 remains the real local player and updates the same object.
    const auto local_get = dispatcher.dispatch(Command{"entity.getPos", {"0"}});
    assert(local_get.has_value());
    (void)dispatcher.dispatch(Command{"entity.setPos", {"0", "1", "2", "3"}});
    assert(game.entities().find(0)->position() == game.player_position());

    // Actual block-hit events retain poll-once and clear semantics.
    game.add_block_hit({{5, 6, 7}, 2, 0});
    auto hits = game.poll_block_hits();
    assert(hits.size() == 1U);
    assert(hits.front().entity_id == 0);
    assert(game.poll_block_hits().empty());
    game.add_block_hit({{1, 2, 3}, 4, 0});
    game.clear_events();
    assert(game.poll_block_hits().empty());

    return 0;
}
