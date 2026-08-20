#include "storage/StorageRouter.hpp"

#include "game/GameState.hpp"
#include "storage/LegacyLevelStorage.hpp"
#include "storage/PiLevelStorage.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <string>

namespace mcpi::storage {
namespace {

constexpr char legacy_magic[] = "MCPI_RECOMPILED_WORLD";

bool has_pi_header(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    std::array<unsigned char, 8> header{};
    input.read(reinterpret_cast<char*>(header.data()),
               static_cast<std::streamsize>(header.size()));
    if (input.gcount() != static_cast<std::streamsize>(header.size())) {
        return false;
    }

    const std::uint32_t version =
        static_cast<std::uint32_t>(header[0]) |
        (static_cast<std::uint32_t>(header[1]) << 8U) |
        (static_cast<std::uint32_t>(header[2]) << 16U) |
        (static_cast<std::uint32_t>(header[3]) << 24U);
    if (version != PiLevelStorage::file_version) {
        return false;
    }

    const std::uint32_t payload_size =
        static_cast<std::uint32_t>(header[4]) |
        (static_cast<std::uint32_t>(header[5]) << 8U) |
        (static_cast<std::uint32_t>(header[6]) << 16U) |
        (static_cast<std::uint32_t>(header[7]) << 24U);

    if (payload_size == 0U || payload_size > PiLevelStorage::max_level_dat_payload) {
        return false;
    }

    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    return !error && size == 8U + payload_size;
}

bool has_legacy_header(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    std::string prefix(sizeof(legacy_magic) - 1U, '\0');
    input.read(prefix.data(), static_cast<std::streamsize>(prefix.size()));
    return input.gcount() == static_cast<std::streamsize>(prefix.size()) &&
           prefix == legacy_magic;
}

} // namespace

StorageFormat storage_format_for_path(
    const std::filesystem::path& path) noexcept {
    const auto extension = path.extension().string();
    if (extension == ".dat") {
        return StorageFormat::PiLevelDat;
    }
    if (extension == ".mcpiworld") {
        return StorageFormat::Legacy;
    }
    return StorageFormat::Legacy;
}

StorageFormat detect_storage_format(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return StorageFormat::Unknown;
    }
    if (has_pi_header(path)) {
        return StorageFormat::PiLevelDat;
    }
    if (has_legacy_header(path)) {
        return StorageFormat::Legacy;
    }
    return StorageFormat::Unknown;
}

bool load_world(game::GameState& state, const std::filesystem::path& path) {
    switch (detect_storage_format(path)) {
    case StorageFormat::Legacy: {
        LegacyLevelStorage storage;
        return storage.load(state, path);
    }
    case StorageFormat::PiLevelDat: {
        PiLevelStorage storage;
        return storage.load(state, path);
    }
    case StorageFormat::Unknown:
        return false;
    }
    return false;
}

bool save_world(const game::GameState& state, const std::filesystem::path& path) {
    StorageFormat format = StorageFormat::Unknown;
    if (std::filesystem::exists(path)) {
        format = detect_storage_format(path);
        if (format == StorageFormat::Unknown) {
            return false;
        }
    } else {
        format = storage_format_for_path(path);
    }

    switch (format) {
    case StorageFormat::Legacy: {
        LegacyLevelStorage storage;
        return storage.save(state, path);
    }
    case StorageFormat::PiLevelDat: {
        PiLevelStorage storage;
        return storage.save(state, path);
    }
    case StorageFormat::Unknown:
        return false;
    }
    return false;
}

} // namespace mcpi::storage
