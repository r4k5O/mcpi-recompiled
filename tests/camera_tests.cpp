#include "client/Camera.hpp"
#include "game/GameState.hpp"

#include <cassert>
#include <cmath>

namespace {
bool near(double a, double b, double epsilon = 1.0e-6) {
    return std::abs(a - b) <= epsilon;
}
}

int main() {
    using mcpi::client::CameraController;
    using mcpi::game::CameraMode;
    using mcpi::game::GameState;

    GameState game;
    game.world().clear();
    game.set_player_position({10.0, 10.0, 10.0});
    game.set_camera_target_entity(0);

    game.set_camera_mode(CameraMode::Normal);
    auto pose = CameraController::resolve(game, 0.0, 0.0);
    assert(near(pose.position.x, 10.0));
    assert(near(pose.position.y, 11.62));
    assert(near(pose.position.z, 10.0));

    game.set_camera_mode(CameraMode::ThirdPerson);
    pose = CameraController::resolve(game, 0.0, 0.0);
    assert(near(pose.position.x, 10.0));
    assert(pose.position.z < 10.0);
    assert(near(pose.position.z, 6.0, 0.15));

    // A solid block behind the target must shorten the third-person boom.
    game.set_block(10, 11, 8, 1, 0);
    const auto clipped = CameraController::resolve(game, 0.0, 0.0);
    assert(clipped.position.z > pose.position.z);
    assert(clipped.position.z > 8.0);

    game.set_camera_position({20.0, 30.0, 40.0});
    game.set_camera_mode(CameraMode::Fixed);
    pose = CameraController::resolve(game, 0.7, -0.2);
    assert(pose.position == mcpi::game::Vec3{20.0, 30.0, 40.0});

    game.set_camera_target_entity(999);
    game.set_camera_mode(CameraMode::Normal);
    pose = CameraController::resolve(game, 0.0, 0.0);
    assert(near(pose.position.x, game.player_position().x));

    return 0;
}
