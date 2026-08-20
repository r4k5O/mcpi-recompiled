#pragma once

#include <filesystem>

namespace mcpi::game {
class GameState;
}

namespace mcpi::storage {

enum class StorageFormat {
    Unknown,
    Legacy,
    PiLevelDat,
};

[[nodiscard]] StorageFormat storage_format_for_path(
    const std::filesystem::path& path) noexcept;

[[nodiscard]] StorageFormat detect_storage_format(
    const std::filesystem::path& path);

[[nodiscard]] bool load_world(
    game::GameState& state,
    const std::filesystem::path& path);

[[nodiscard]] bool save_world(
    const game::GameState& state,
    const std::filesystem::path& path);

} // namespace mcpi::storage
