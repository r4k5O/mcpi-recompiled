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
#include <sstream>
#include <string>

namespace {

bool close(double a, double b) {
    return std::abs(a - b) < 0.001;
}

} // namespace

int main() {
    using mcpi::game::GameState;
    using mcpi::storage::PiLevelStorage;
    using mcpi::storage::StorageFormat;
    using namespace mcpi::storage::nbt;

    // Little-endian NBT round-trip for the primitive/container families needed
    // by the original PE/Pi level.dat metadata.
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

    // The payload integer 0x01020304 must physically be 04 03 02 01 in the
    // little-endian Pi/PE dialect, not Java Edition's big-endian ordering.
    std::ostringstream int_stream(std::ios::binary);
    assert(write_named(int_stream, "I", Tag::integer(0x01020304), Endian::Little));
    const std::string int_bytes = int_stream.str();
    assert(int_bytes.size() == 8U);
    assert(static_cast<unsigned char>(int_bytes[4]) == 0x04U);
    assert(static_cast<unsigned char>(int_bytes[5]) == 0x03U);
    assert(static_cast<unsigned char>(int_bytes[6]) == 0x02U);
    assert(static_cast<unsigned char>(int_bytes[7]) == 0x01U);

    // Malformed/truncated input and hostile container lengths must be rejected
    // rather than allocating unbounded memory or accepting partial payloads.
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

    // Pi level.dat metadata: 8-byte little-endian header + NBT containing the
    // confirmed SpawnX/Y/Z, RandomSeed and Player.Pos fields.
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

    GameState loaded;
    assert(storage.load(loaded, path));
    assert(loaded.seed() == source.seed());
    assert(loaded.spawn_position() == source.spawn_position());
    assert(close(loaded.player_position().x, source.player_position().x));
    assert(close(loaded.player_position().y, source.player_position().y));
    assert(close(loaded.player_position().z, source.player_position().z));

    // Routing must retain Phase-1 legacy saves while selecting Pi storage for
    // level.dat. Existing files are sniffed so renaming a file does not corrupt
    // it by silently choosing the wrong reader.
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

    source.set_block(20, 70, 20, 57, 6);
    assert(mcpi::storage::save_world(source, legacy_path));
    GameState legacy_loaded;
    assert(mcpi::storage::load_world(legacy_loaded, legacy_path));
    assert(legacy_loaded.block_type(20, 70, 20) == 57);
    assert(legacy_loaded.block_data(20, 70, 20) == 6);

    assert(mcpi::storage::save_world(source, pi_path));
    std::filesystem::copy_file(
        pi_path,
        renamed_pi_path,
        std::filesystem::copy_options::overwrite_existing);
    assert(mcpi::storage::detect_storage_format(renamed_pi_path) ==
           StorageFormat::PiLevelDat);
    GameState sniffed_loaded;
    assert(mcpi::storage::load_world(sniffed_loaded, renamed_pi_path));
    assert(sniffed_loaded.seed() == source.seed());
    assert(sniffed_loaded.spawn_position() == source.spawn_position());

    std::filesystem::remove(path);
    std::filesystem::remove(legacy_path);
    std::filesystem::remove(pi_path);
    std::filesystem::remove(renamed_pi_path);
    return 0;
}
