#pragma once

#include "game/GameApi.hpp"

#include <algorithm>
#include <cmath>

namespace mcpi::client {

struct CameraPose {
    game::Vec3 position{};
    double yaw = 0.0;
    double pitch = 0.0;
    int target_entity_id = 0;
};

class CameraController {
public:
    static constexpr double eye_height = 1.62;
    static constexpr double third_person_distance = 4.0;

    [[nodiscard]] static CameraPose resolve(const game::GameApi& game,
                                            double yaw,
                                            double pitch) {
        const int target_id = game.camera_target_entity();
        game::Vec3 target{};
        if (!game.entity_position(target_id, target)) {
            target = game.player_position();
        }

        const game::Vec3 eye{target.x, target.y + eye_height, target.z};
        CameraPose pose{eye, yaw, pitch, target_id};

        if (game.camera_mode() == game::CameraMode::Fixed) {
            pose.position = game.camera_position();
            return pose;
        }

        if (game.camera_mode() == game::CameraMode::Normal) {
            return pose;
        }

        const double cp = std::cos(pitch);
        const game::Vec3 forward{
            std::sin(yaw) * cp,
            std::sin(pitch),
            std::cos(yaw) * cp,
        };

        double usable_distance = third_person_distance;
        constexpr double step = 0.10;
        constexpr double clearance = 0.20;
        for (double distance = step; distance <= third_person_distance; distance += step) {
            const game::Vec3 sample{
                eye.x - forward.x * distance,
                eye.y - forward.y * distance,
                eye.z - forward.z * distance,
            };
            const int bx = static_cast<int>(std::floor(sample.x));
            const int by = static_cast<int>(std::floor(sample.y));
            const int bz = static_cast<int>(std::floor(sample.z));
            if (game.block_type(bx, by, bz) != 0) {
                usable_distance = std::max(0.0, distance - clearance);
                break;
            }
        }

        pose.position = {
            eye.x - forward.x * usable_distance,
            eye.y - forward.y * usable_distance,
            eye.z - forward.z * usable_distance,
        };
        return pose;
    }
};

} // namespace mcpi::client
