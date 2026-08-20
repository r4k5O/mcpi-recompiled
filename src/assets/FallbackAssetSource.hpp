#pragma once

#include "assets/AssetSource.hpp"

namespace mcpi::assets {

class FallbackAssetSource final : public AssetSource {
public:
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> read(
        std::string_view path) const override;
};

} // namespace mcpi::assets
