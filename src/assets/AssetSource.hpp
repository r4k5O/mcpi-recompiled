#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace mcpi::assets {

class AssetSource {
public:
    virtual ~AssetSource() = default;
    [[nodiscard]] virtual std::optional<std::vector<std::uint8_t>> read(
        std::string_view path) const = 0;
};

} // namespace mcpi::assets
