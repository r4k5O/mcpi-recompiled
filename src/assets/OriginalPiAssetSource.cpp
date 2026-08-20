#include "assets/OriginalPiAssetSource.hpp"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <system_error>

namespace mcpi::assets {
namespace {

std::optional<std::filesystem::path> canonical_directory(const std::filesystem::path& candidate) {
    std::error_code ec;
    if (!std::filesystem::is_directory(candidate, ec) || ec) {
        return std::nullopt;
    }
    auto canonical = std::filesystem::weakly_canonical(candidate, ec);
    if (ec) {
        return std::nullopt;
    }
    return canonical;
}

} // namespace

OriginalPiAssetSource::OriginalPiAssetSource(std::filesystem::path root) {
    std::error_code ec;
    root_ = std::filesystem::weakly_canonical(std::move(root), ec);
    if (ec) {
        root_.clear();
    }
}

bool OriginalPiAssetSource::safe_relative_path(const std::filesystem::path& path) noexcept {
    if (path.empty() || path.has_root_name() || path.has_root_directory() || path.is_absolute()) {
        return false;
    }
    for (const auto& component : path) {
        if (component == "..") {
            return false;
        }
    }
    return true;
}

bool OriginalPiAssetSource::contained(const std::filesystem::path& candidate) const {
    if (root_.empty()) {
        return false;
    }
    std::error_code ec;
    const auto relative = std::filesystem::relative(candidate, root_, ec);
    if (ec || relative.empty()) {
        return !ec && candidate == root_;
    }
    if (relative.has_root_name() || relative.has_root_directory() || relative.is_absolute()) {
        return false;
    }
    for (const auto& component : relative) {
        if (component == "..") {
            return false;
        }
    }
    return true;
}

std::optional<std::vector<std::uint8_t>> OriginalPiAssetSource::read(std::string_view raw_path) const {
    const std::filesystem::path relative{std::string(raw_path)};
    if (!safe_relative_path(relative) || root_.empty()) {
        return std::nullopt;
    }

    std::error_code ec;
    const auto candidate = std::filesystem::weakly_canonical(root_ / relative, ec);
    if (ec || !contained(candidate) || !std::filesystem::is_regular_file(candidate, ec) || ec) {
        return std::nullopt;
    }

    std::ifstream input(candidate, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                     std::istreambuf_iterator<char>());
}

const std::filesystem::path& OriginalPiAssetSource::root() const noexcept {
    return root_;
}

std::optional<std::filesystem::path> OriginalPiAssetSource::locate(
    const std::optional<std::filesystem::path>& explicit_root) {
    if (explicit_root.has_value()) {
        return canonical_directory(*explicit_root);
    }

    if (const char* environment = std::getenv("MCPI_ASSETS"); environment != nullptr && *environment != '\0') {
        if (auto root = canonical_directory(environment)) {
            return root;
        }
    }

    const std::filesystem::path candidates[] = {
        std::filesystem::current_path() / "mcpi",
        std::filesystem::current_path() / "minecraft-pi",
        std::filesystem::current_path() / "assets",
        "/opt/minecraft-pi",
    };
    for (const auto& candidate : candidates) {
        if (auto root = canonical_directory(candidate)) {
            return root;
        }
    }

    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        if (auto root = canonical_directory(std::filesystem::path(home) / "minecraft-pi")) {
            return root;
        }
    }
    return std::nullopt;
}

} // namespace mcpi::assets
