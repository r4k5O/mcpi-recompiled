#include "storage/PiLevelStorage.hpp"

#include "game/GameState.hpp"
#include "storage/Nbt.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

namespace mcpi::storage {
namespace {

using nbt::Tag;
using nbt::Type;

bool write_u32_le(std::ostream& output, std::uint32_t value) {
    for (unsigned shift = 0U; shift < 32U; shift += 8U) {
        output.put(static_cast<char>((value >> shift) & 0xffU));
        if (!output) {
            return false;
        }
    }
    return true;
}

bool read_u32_le(std::istream& input, std::uint32_t& value) {
    value = 0U;
    for (unsigned shift = 0U; shift < 32U; shift += 8U) {
        char raw = 0;
        if (!input.get(raw)) {
            return false;
        }
        value |= static_cast<std::uint32_t>(static_cast<unsigned char>(raw))
                 << shift;
    }
    return true;
}

const Tag* find_tag(const Tag& compound, const char* name, Type type) {
    if (compound.type != Type::Compound) {
        return nullptr;
    }
    const auto found = compound.compound_value.find(name);
    if (found == compound.compound_value.end() || found->second.type != type) {
        return nullptr;
    }
    return &found->second;
}

bool seed_to_u32(std::int64_t value, std::uint32_t& result) {
    if (value >= 0 &&
        static_cast<std::uint64_t>(value) <=
            std::numeric_limits<std::uint32_t>::max()) {
        result = static_cast<std::uint32_t>(value);
        return true;
    }
    if (value >= std::numeric_limits<std::int32_t>::min() && value < 0) {
        result = static_cast<std::uint32_t>(static_cast<std::int32_t>(value));
        return true;
    }
    return false;
}

} // namespace

bool PiLevelStorage::save(
    const game::GameState& state,
    const std::filesystem::path& path) {
    const auto spawn = state.spawn_position();
    const auto player = state.player_position();

    Tag root = Tag::compound({
        {"Player", Tag::compound({
            {"Pos", Tag::list(Type::Float, {
                Tag::floating(static_cast<float>(player.x)),
                Tag::floating(static_cast<float>(player.y)),
                Tag::floating(static_cast<float>(player.z)),
            })},
        })},
        {"RandomSeed", Tag::long_integer(static_cast<std::int64_t>(state.seed()))},
        {"SpawnX", Tag::integer(spawn.x)},
        {"SpawnY", Tag::integer(spawn.y)},
        {"SpawnZ", Tag::integer(spawn.z)},
        {"StorageVersion", Tag::integer(static_cast<std::int32_t>(file_version))},
    });

    std::ostringstream payload(std::ios::binary);
    if (!nbt::write_named(payload, "", root, nbt::Endian::Little)) {
        return false;
    }
    const std::string bytes = payload.str();
    if (bytes.empty() || bytes.size() > max_level_dat_payload ||
        bytes.size() > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output ||
        !write_u32_le(output, file_version) ||
        !write_u32_le(output, static_cast<std::uint32_t>(bytes.size()))) {
        return false;
    }
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(output);
}

bool PiLevelStorage::load(
    game::GameState& state,
    const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }

    std::uint32_t version = 0U;
    std::uint32_t payload_size = 0U;
    if (!read_u32_le(input, version) ||
        !read_u32_le(input, payload_size) ||
        version != file_version ||
        payload_size == 0U ||
        payload_size > max_level_dat_payload) {
        return false;
    }

    std::error_code file_size_error;
    const auto file_size = std::filesystem::file_size(path, file_size_error);
    if (file_size_error || file_size != 8U + payload_size) {
        return false;
    }

    std::string payload(payload_size, '\0');
    input.read(payload.data(), static_cast<std::streamsize>(payload.size()));
    if (input.gcount() != static_cast<std::streamsize>(payload.size())) {
        return false;
    }

    std::istringstream nbt_input(payload, std::ios::binary);
    std::string root_name;
    Tag root;
    if (!nbt::read_named(
            nbt_input, root_name, root, nbt::Endian::Little) ||
        root.type != Type::Compound ||
        nbt_input.peek() != std::char_traits<char>::eof()) {
        return false;
    }

    const Tag* random_seed = find_tag(root, "RandomSeed", Type::Long);
    const Tag* spawn_x = find_tag(root, "SpawnX", Type::Int);
    const Tag* spawn_y = find_tag(root, "SpawnY", Type::Int);
    const Tag* spawn_z = find_tag(root, "SpawnZ", Type::Int);
    const Tag* player = find_tag(root, "Player", Type::Compound);
    if (random_seed == nullptr || spawn_x == nullptr || spawn_y == nullptr ||
        spawn_z == nullptr || player == nullptr) {
        return false;
    }

    const Tag* storage_version = find_tag(root, "StorageVersion", Type::Int);
    if (storage_version != nullptr &&
        storage_version->int_value != static_cast<std::int32_t>(file_version)) {
        return false;
    }

    const Tag* position = find_tag(*player, "Pos", Type::List);
    if (position == nullptr || position->list_type != Type::Float ||
        position->list_value.size() != 3U) {
        return false;
    }
    for (const auto& coordinate : position->list_value) {
        if (coordinate.type != Type::Float) {
            return false;
        }
    }

    std::uint32_t seed = 0U;
    if (!seed_to_u32(random_seed->long_value, seed)) {
        return false;
    }

    state.new_world(seed);
    state.set_spawn_position({
        spawn_x->int_value,
        spawn_y->int_value,
        spawn_z->int_value,
    });
    state.set_player_position({
        static_cast<double>(position->list_value[0].float_value),
        static_cast<double>(position->list_value[1].float_value),
        static_cast<double>(position->list_value[2].float_value),
    });
    return true;
}

} // namespace mcpi::storage
