#include "assets/FallbackAssetSource.hpp"

namespace mcpi::assets {

std::optional<std::vector<std::uint8_t>> FallbackAssetSource::read(std::string_view path) const {
    if (path != "fallback/checker.rgba") {
        return std::nullopt;
    }

    std::vector<std::uint8_t> pixels;
    pixels.reserve(4U * 4U * 4U);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            const bool bright = ((x + y) & 1) == 0;
            pixels.push_back(bright ? 255U : 32U);
            pixels.push_back(bright ? 64U : 32U);
            pixels.push_back(bright ? 255U : 32U);
            pixels.push_back(255U);
        }
    }
    return pixels;
}

} // namespace mcpi::assets
