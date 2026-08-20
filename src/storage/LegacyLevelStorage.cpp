#include "storage/LegacyLevelStorage.hpp"

#include "game/GameState.hpp"

namespace mcpi::storage {

bool LegacyLevelStorage::load(
    game::GameState& state,
    const std::filesystem::path& path) {
    return state.load(path);
}

bool LegacyLevelStorage::save(
    const game::GameState& state,
    const std::filesystem::path& path) {
    return state.save(path);
}

} // namespace mcpi::storage
