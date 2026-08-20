#pragma once

#include "assets/AssetSource.hpp"

#include <filesystem>
#include <optional>

namespace mcpi::assets {

class OriginalPiAssetSource final : public AssetSource {
public:
    explicit OriginalPiAssetSource(std::filesystem::path root);

    [[nodiscard]] std::optional<std::vector<std::uint8_t>> read(
        std::string_view path) const override;
    [[nodiscard]] const std::filesystem::path& root() const noexcept;

    [[nodiscard]] static std::optional<std::filesystem::path> locate(
        const std::optional<std::filesystem::path>& explicit_root = std::nullopt);

private:
    [[nodiscard]] static bool safe_relative_path(const std::filesystem::path& path) noexcept;
    [[nodiscard]] bool contained(const std::filesystem::path& candidate) const;

    std::filesystem::path root_;
};

} // namespace mcpi::assets
