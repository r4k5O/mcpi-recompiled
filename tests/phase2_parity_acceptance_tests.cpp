#include "api/ApiDispatcher.hpp"
#include "api/Command.hpp"
#include "client/Camera.hpp"
#include "client/ChunkMesh.hpp"
#include "client/LevelRenderer.hpp"
#include "game/GameLoop.hpp"
#include "game/GameState.hpp"
#include "parity/ReferenceSuite.hpp"
#include "storage/StorageRouter.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

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

std::uint64_t world_light_fingerprint(const mcpi::game::GameState& game) {
    std::uint64_t hash = 0x84222325cbf29ce4ULL;
    for (int x = 128; x < 132; ++x) {
        for (int z = 128; z < 132; ++z) {
            for (const int y : {0, 32, 64, 96, 127}) {
                const mcpi::world::BlockPos position{x, y, z};
                const auto block = game.world().block_at(position);
                hash = mix(hash, static_cast<std::uint64_t>(block.id));
                hash = mix(hash, static_cast<std::uint64_t>(block.data));
                hash = mix(hash, static_cast<std::uint64_t>(game.world().sky_light_at(position)));
            }
        }
    }
    return hash;
}

std::uint64_t frame_digest(const mcpi::client::RenderFrame& frame) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    hash = mix(hash, static_cast<std::uint64_t>(frame.width));
    hash = mix(hash, static_cast<std::uint64_t>(frame.height));
    for (const auto byte : frame.rgba) {
        hash = mix(hash, static_cast<std::uint64_t>(byte));
    }
    return hash;
}

std::vector<std::string> split_transcript(std::string_view text) {
    std::vector<std::string> commands;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto separator = text.find('|', start);
        const auto end = separator == std::string_view::npos ? text.size() : separator;
        commands.emplace_back(text.substr(start, end - start));
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1U;
    }
    return commands;
}

std::string encode(const mcpi::api::CommandResult& result) {
    using mcpi::api::CommandResultKind;
    switch (result.kind) {
    case CommandResultKind::Response:
        return "response:" + result.response;
    case CommandResultKind::NoResponse:
        return "no-response";
    case CommandResultKind::Fail:
        return "fail";
    }
    return "invalid";
}

void verify_transcript_suite(const std::filesystem::path& path) {
    const auto suite = mcpi::parity::ReferenceSuite::load(path);
    const auto cases = suite.cases("api-transcript");
    assert(!cases.empty());

    for (const auto& reference : cases) {
        mcpi::game::GameState game;
        game.world().clear();
        game.set_spawn_position({0, 0, 0});
        game.set_player_position({0.0, 0.0, 0.0});
        if (reference.name == "spawn-offset") {
            game.set_spawn_position({10, 20, 30});
        }

        mcpi::api::ApiDispatcher dispatcher(game);
        mcpi::api::CommandResult result = mcpi::api::CommandResult::no_response();
        for (const auto& line : split_transcript(reference.input)) {
            const auto command = mcpi::api::parse_command(line);
            assert(command.has_value());
            result = dispatcher.dispatch_result(*command);
        }
        assert(encode(result) == reference.expected);
    }
}
}

int main(int argc, char** argv) {
    assert(argc == 2);

    using mcpi::api::ApiDispatcher;
    using mcpi::api::Command;
    using mcpi::client::CameraController;
    using mcpi::client::ChunkMeshBuilder;
    using mcpi::client::LevelRenderer;
    using mcpi::game::CameraMode;
    using mcpi::game::GameLoop;
    using mcpi::game::GameState;

    GameState game;
    game.new_world(424242U);

    GameState repeated_world;
    repeated_world.new_world(424242U);
    const auto generation_fingerprint = world_light_fingerprint(game);
    assert(generation_fingerprint != 0U);
    assert(generation_fingerprint == world_light_fingerprint(repeated_world));

    game.set_spawn_position({128, 64, 128});
    game.set_player_position({128.5, 120.0, 128.5});
    game.player().set_velocity({0.25, 0.0, 0.0});
    const double before_tick_x = game.player_position().x;

    GameLoop loop;
    assert(loop.advance(game, GameLoop::provisional_tick_seconds) == 1U);
    assert(loop.tick_count() == 1U);
    assert(game.player_position().x > before_tick_x);
    assert(loop.last_tick_trace().size() == 2U);
    assert(loop.last_tick_trace()[0] == mcpi::game::TickStage::PlayerPhysics);
    assert(loop.last_tick_trace()[1] == mcpi::game::TickStage::ScheduledBlockUpdates);

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

    verify_transcript_suite(std::filesystem::path(argv[1]));

    game.set_camera_target_entity(0);
    game.set_camera_mode(CameraMode::ThirdPerson);
    const auto camera = CameraController::resolve(game, 0.0, -0.25);
    assert(camera.position.z < game.player_position().z);

    ChunkMeshBuilder mesh_builder;
    const auto mesh = mesh_builder.build(game.world(), 8, 8);
    assert(!mesh.opaque.empty() || !mesh.translucent.empty());

    const auto light = game.world().sky_light_at({130, 65, 130});
    assert(light <= 15U);

    LevelRenderer renderer;
    mcpi::client::CameraPose render_camera;
    render_camera.position = {128.0, 90.0, 110.0};
    render_camera.yaw = 0.0;
    render_camera.pitch = -0.35;
    const auto frame_a = renderer.render(game.world(), render_camera, 96, 64, 2);
    const auto frame_b = renderer.render(game.world(), render_camera, 96, 64, 2);
    assert(frame_a.rgba == frame_b.rgba);
    assert(frame_digest(frame_a) == frame_digest(frame_b));

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
