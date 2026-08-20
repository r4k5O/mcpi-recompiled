#include "api/ApiDispatcher.hpp"
#include "client/Camera.hpp"
#include "client/ChunkMesh.hpp"
#include "game/GameState.hpp"
#include "storage/StorageRouter.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

namespace {
std::uint64_t mix(std::uint64_t hash, std::uint64_t value) {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
    return hash;
}

std::uint64_t persisted_digest(const mcpi::game::GameState& game) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    hash = mix(hash, game.seed());
    const auto block = game.world().block_at({130, 65, 130});
    hash = mix(hash, static_cast<std::uint64_t>(block.id));
    hash = mix(hash, static_cast<std::uint64_t>(block.data));
    const auto position = game.player_position();
    hash = mix(hash, static_cast<std::uint64_t>(position.x * 1000.0));
    hash = mix(hash, static_cast<std::uint64_t>(position.y * 1000.0));
    hash = mix(hash, static_cast<std::uint64_t>(position.z * 1000.0));
    return hash;
}
}

int main() {
    using mcpi::api::ApiDispatcher;
    using mcpi::api::Command;
    using mcpi::client::CameraController;
    using mcpi::client::ChunkMeshBuilder;
    using mcpi::game::CameraMode;
    using mcpi::game::GameState;

    GameState game;
    game.new_world(424242U);
    game.set_spawn_position({128, 64, 128});
    game.set_player_position({128.5, 65.0, 128.5});
    game.set_hotbar_block(2, 57);
    game.select_hotbar_slot(2);
    assert(game.selected_block() == 57);

    mcpi::game::Vec3 entity_position{};
    assert(game.entity_position(0, entity_position));
    assert(entity_position == game.player_position());

    ApiDispatcher dispatcher(game);
    (void)dispatcher.dispatch(Command{"world.setBlock", {"2", "1", "2", "41", "0"}});
    const auto response = dispatcher.dispatch(Command{"world.getBlock", {"2", "1", "2"}});
    assert(response.has_value() && *response == "41");

    game.set_camera_target_entity(0);
    game.set_camera_mode(CameraMode::ThirdPerson);
    const auto camera = CameraController::resolve(game, 0.0, 0.0);
    assert(camera.position.z < game.player_position().z);

    ChunkMeshBuilder mesh_builder;
    const auto mesh = mesh_builder.build(game.world(), 8, 8);
    assert(!mesh.opaque.empty() || !mesh.translucent.empty());

    const auto light = game.world().sky_light_at({130, 65, 130});
    assert(light <= 15U);

    const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
                      ("mcpi-phase2-" + std::to_string(nonce) + ".mcpiworld");
    assert(mcpi::storage::save_world(game, path));
    const auto before = persisted_digest(game);

    GameState loaded;
    assert(mcpi::storage::load_world(loaded, path));
    const auto after = persisted_digest(loaded);
    std::filesystem::remove(path);
    assert(before == after);

    return 0;
}
