#include "game/GameState.hpp"
#include "storage/Nbt.hpp"
#include "storage/PiLevelStorage.hpp"
#include "storage/StorageRouter.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

bool close(double a, double b) {
    return std::abs(a - b) < 0.001;
}

void stage(const char* name) {
    std::cerr << "[storage_parity] " << name << std::endl;
}

bool copy_binary_file(const std::filesystem::path& source,
                      const std::filesystem::path& destination) {
    std::ifstream input(source, std::ios::binary);
    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!input || !output) {
        return false;
    }
    output << input.rdbuf();
    return static_cast<bool>(input) && static_cast<bool>(output);
}

} // namespace

int main() {
    using mcpi::game::GameState;
    using mcpi::storage::PiLevelStorage;
    using mcpi::storage::StorageFormat;
    using namespace mcpi::storage::nbt;

    stage("nbt-roundtrip");
    Tag root = Tag::compound({
        {"Flag", Tag::byte(1)},
        {"Count", Tag::integer(0x01020304)},
        {"Name", Tag::string("Minecraft Pi")},
        {"Values", Tag::list(Type::Int, {
            Tag::integer(1),
            Tag::integer(-2),
            Tag::integer(3),
        })},
        {"Nested", Tag::compound({
            {"Answer", Tag::integer(42)},
        })},
    });

    std::ostringstream encoded(std::ios::binary);
    assert(write_named(encoded, "", root, Endian::Little));

    std::istringstream decoded(encoded.str(), std::ios::binary);
    std::string root_name;
    Tag round_tripped;
    assert(read_named(decoded, root_name, round_tripped, Endian::Little));
    assert(root_name.empty());
    assert(round_tripped == root);

    stage("little-endian-int");
    std::ostringstream int_stream(std::ios::binary);
    assert(write_named(int_stream, "I", Tag::integer(0x01020304), Endian::Little));
    const std::string int_bytes = int_stream.str();
    assert(int_bytes.size() == 8U);
    assert(static_cast<unsigned char>(int_bytes[4]) == 0x04U);
    assert(static_cast<unsigned char>(int_bytes[5]) == 0x03U);
    assert(static_cast<unsigned char>(int_bytes[6]) == 0x02U);
    assert(static_cast<unsigned char>(int_bytes[7]) == 0x01U);

    stage("malformed-input");
    std::string malformed = encoded.str();
    malformed.resize(malformed.size() / 2U);
    std::istringstream truncated(malformed, std::ios::binary);
    Tag rejected;
    std::string rejected_name;
    assert(!read_named(truncated, rejected_name, rejected, Endian::Little));

    const std::array<unsigned char, 8> hostile_list{{
        0x09,
        0x00, 0x00,
        0x03,
        0xff, 0xff, 0xff, 0x7f
    }};
    const std::string hostile_bytes(
        reinterpret_cast<const char*>(hostile_list.data()), hostile_list.size());
    std::istringstream hostile(hostile_bytes, std::ios::binary);
    assert(!read_named(hostile, rejected_name, rejected, Endian::Little));

    stage("pi-level-save-header");
    GameState source;
    source.new_world(0x12345678U);
    source.set_spawn_position({12, 70, 34});
    source.set_player_position({11.25, 72.5, 33.75});

    const auto temp = std::filesystem::temp_directory_path();
    const auto path = temp / "mcpi-recompiled-storage-parity-level.dat";
    std::filesystem::remove(path);

    PiLevelStorage storage;
    assert(storage.save(source, path));

    std::ifstream raw(path, std::ios::binary);
    assert(raw);
    std::array<unsigned char, 8> header{};
    raw.read(reinterpret_cast<char*>(header.data()),
             static_cast<std::streamsize>(header.size()));
    assert(raw.gcount() == static_cast<std::streamsize>(header.size()));
    assert(header[0] == 0x03U && header[1] == 0x00U &&
           header[2] == 0x00U && header[3] == 0x00U);

    const std::uint32_t payload_size =
        static_cast<std::uint32_t>(header[4]) |
        (static_cast<std::uint32_t>(header[5]) << 8U) |
        (static_cast<std::uint32_t>(header[6]) << 16U) |
        (static_cast<std::uint32_t>(header[7]) << 24U);
    assert(std::filesystem::file_size(path) == 8U + payload_size);

    stage("pi-level-load");
    GameState loaded;
    assert(storage.load(loaded, path));
    assert(loaded.seed() == source.seed());
    assert(loaded.spawn_position() == source.spawn_position());
    assert(close(loaded.player_position().x, source.player_position().x));
    assert(close(loaded.player_position().y, source.player_position().y));
    assert(close(loaded.player_position().z, source.player_position().z));

    stage("router-path-selection");
    assert(mcpi::storage::storage_format_for_path("world.mcpiworld") ==
           StorageFormat::Legacy);
    assert(mcpi::storage::storage_format_for_path("level.dat") ==
           StorageFormat::PiLevelDat);

    const auto legacy_path = temp / "mcpi-recompiled-storage-router.mcpiworld";
    const auto pi_path = temp / "mcpi-recompiled-storage-router.dat";
    const auto renamed_pi_path = temp / "mcpi-recompiled-storage-router.bin";
    std::filesystem::remove(legacy_path);
    std::filesystem::remove(pi_path);
    std::filesystem::remove(renamed_pi_path);

    stage("router-legacy-roundtrip");
    source.set_block(20, 70, 20, 57, 6);
    assert(mcpi::storage::save_world(source, legacy_path));
    GameState legacy_loaded;
    assert(mcpi::storage::load_world(legacy_loaded, legacy_path));
    assert(legacy_loaded.block_type(20, 70, 20) == 57);
    assert(legacy_loaded.block_data(20, 70, 20) == 6);

    stage("router-pi-save");
    assert(mcpi::storage::save_world(source, pi_path));
    stage("router-pi-copy");
    assert(copy_binary_file(pi_path, renamed_pi_path));
    stage("router-pi-detect");
    const auto detected = mcpi::storage::detect_storage_format(renamed_pi_path);
    std::cerr << "[storage_parity] detected=" << static_cast<int>(detected) << std::endl;
    assert(detected == StorageFormat::PiLevelDat);
    stage("router-pi-load");
    GameState sniffed_loaded;
    const bool sniffed_ok = mcpi::storage::load_world(sniffed_loaded, renamed_pi_path);
    std::cerr << "[storage_parity] load=" << (sniffed_ok ? 1 : 0) << std::endl;
    assert(sniffed_ok);
    stage("router-pi-seed");
    assert(sniffed_loaded.seed() == source.seed());
    stage("router-pi-spawn");
    assert(sniffed_loaded.spawn_position() == source.spawn_position());

    stage("cleanup");
    std::filesystem::remove(path);
    std::filesystem::remove(legacy_path);
    std::filesystem::remove(pi_path);
    std::filesystem::remove(renamed_pi_path);
    stage("done");
    return 0;
}
