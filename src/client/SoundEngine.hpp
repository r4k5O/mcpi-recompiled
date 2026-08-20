#pragma once

#include "assets/AssetSource.hpp"
#include "game/GameApi.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mcpi::client {

struct PendingSound {
    std::string event;
    std::vector<std::uint8_t> encoded;
    float gain = 1.0f;
};

class SoundEngine {
public:
    explicit SoundEngine(const assets::AssetSource& assets);

    [[nodiscard]] bool play(
        std::string_view event,
        const game::Vec3& origin,
        const game::Vec3& listener,
        double max_distance = 16.0);

    [[nodiscard]] std::optional<PendingSound> take_pending();

    [[nodiscard]] static float attenuation(
        const game::Vec3& listener,
        const game::Vec3& origin,
        double max_distance) noexcept;

private:
    [[nodiscard]] static bool valid_event(std::string_view event) noexcept;
    [[nodiscard]] static std::string event_path(std::string_view event);

    const assets::AssetSource& assets_;
    std::optional<PendingSound> pending_;
};

} // namespace mcpi::client
