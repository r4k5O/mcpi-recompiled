#include "client/SoundEngine.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>

namespace mcpi::client {

SoundEngine::SoundEngine(const assets::AssetSource& assets) : assets_(assets) {}

bool SoundEngine::valid_event(std::string_view event) noexcept {
    if (event.empty() || event.size() > 96U) {
        return false;
    }
    return std::all_of(event.begin(), event.end(), [](unsigned char value) {
        return std::isalnum(value) != 0 || value == '.' || value == '_' || value == '-';
    });
}

std::string SoundEngine::event_path(std::string_view event) {
    std::string path = "sounds/";
    for (char value : event) {
        path.push_back(value == '.' ? '/' : value);
    }
    path += ".wav";
    return path;
}

float SoundEngine::attenuation(const game::Vec3& listener,
                               const game::Vec3& origin,
                               double max_distance) noexcept {
    if (max_distance <= 0.0) {
        return 0.0f;
    }
    const double dx = origin.x - listener.x;
    const double dy = origin.y - listener.y;
    const double dz = origin.z - listener.z;
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    return static_cast<float>(std::clamp(1.0 - distance / max_distance, 0.0, 1.0));
}

bool SoundEngine::play(std::string_view event,
                       const game::Vec3& origin,
                       const game::Vec3& listener,
                       double max_distance) {
    pending_.reset();
    if (!valid_event(event)) {
        return false;
    }
    const float gain = attenuation(listener, origin, max_distance);
    if (gain <= 0.0f) {
        return false;
    }

    auto encoded = assets_.read(event_path(event));
    if (!encoded.has_value()) {
        const std::string flat_path = "sounds/" + std::string(event) + ".wav";
        encoded = assets_.read(flat_path);
    }
    if (!encoded.has_value()) {
        return false;
    }

    pending_ = PendingSound{std::string(event), std::move(*encoded), gain};
    return true;
}

std::optional<PendingSound> SoundEngine::take_pending() {
    auto result = std::move(pending_);
    pending_.reset();
    return result;
}

} // namespace mcpi::client
