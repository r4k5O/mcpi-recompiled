#include "api/ApiDispatcher.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace mcpi::api {
namespace {

constexpr std::string_view kRequestFailed = "Fail";

bool parse_int(std::string_view text, int& value) {
    if (text.empty()) {
        return false;
    }

    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parse_double(const std::string& text, double& value) {
    if (text.empty()) {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    value = std::strtod(text.c_str(), &end);
    return errno != ERANGE && end != text.c_str() && end != nullptr && *end == '\0';
}

std::string format_double(double value) {
    std::ostringstream output;
    output << std::setprecision(15) << value;
    return output.str();
}

std::string format_vec3(const game::Vec3& position) {
    return format_double(position.x) + "," +
           format_double(position.y) + "," +
           format_double(position.z);
}

std::string format_ivec3(const game::IVec3& position) {
    return std::to_string(position.x) + "," +
           std::to_string(position.y) + "," +
           std::to_string(position.z);
}

std::optional<std::string> failed_request() {
    return std::string(kRequestFailed);
}

game::Vec3 incoming(const game::Vec3& api, const game::IVec3& spawn) {
    return {
        api.x + static_cast<double>(spawn.x),
        api.y + static_cast<double>(spawn.y),
        api.z + static_cast<double>(spawn.z),
    };
}

game::IVec3 incoming(const game::IVec3& api, const game::IVec3& spawn) {
    return {api.x + spawn.x, api.y + spawn.y, api.z + spawn.z};
}

game::Vec3 outgoing(const game::Vec3& internal, const game::IVec3& spawn) {
    return {
        internal.x - static_cast<double>(spawn.x),
        internal.y - static_cast<double>(spawn.y),
        internal.z - static_cast<double>(spawn.z),
    };
}

game::IVec3 outgoing(const game::IVec3& internal, const game::IVec3& spawn) {
    return {internal.x - spawn.x, internal.y - spawn.y, internal.z - spawn.z};
}

bool parse_ivec3(const std::vector<std::string>& arguments,
                 std::size_t offset,
                 game::IVec3& value) {
    return offset + 2U < arguments.size() &&
           parse_int(arguments[offset], value.x) &&
           parse_int(arguments[offset + 1U], value.y) &&
           parse_int(arguments[offset + 2U], value.z);
}

bool parse_vec3(const std::vector<std::string>& arguments,
                std::size_t offset,
                game::Vec3& value) {
    return offset + 2U < arguments.size() &&
           parse_double(arguments[offset], value.x) &&
           parse_double(arguments[offset + 1U], value.y) &&
           parse_double(arguments[offset + 2U], value.z);
}

} // namespace

ApiDispatcher::ApiDispatcher(game::GameApi& game)
    : game_(game) {}

std::optional<std::string> ApiDispatcher::dispatch(const Command& command) const {
    const game::IVec3 spawn = game_.spawn_position();

    if (command.name == "player.getPos") {
        if (!command.arguments.empty()) {
            return failed_request();
        }
        return format_vec3(outgoing(game_.player_position(), spawn));
    }

    if (command.name == "player.setPos") {
        if (command.arguments.size() != 3U) {
            return std::nullopt;
        }
        game::Vec3 position;
        if (!parse_vec3(command.arguments, 0U, position)) {
            return std::nullopt;
        }
        game_.set_player_position(incoming(position, spawn));
        return std::nullopt;
    }

    if (command.name == "player.getTile") {
        if (!command.arguments.empty()) {
            return failed_request();
        }
        const auto exact = game_.player_position();
        const game::IVec3 tile{
            static_cast<int>(std::floor(exact.x)),
            static_cast<int>(std::floor(exact.y)),
            static_cast<int>(std::floor(exact.z)),
        };
        return format_ivec3(outgoing(tile, spawn));
    }

    if (command.name == "player.setTile") {
        if (command.arguments.size() != 3U) {
            return std::nullopt;
        }
        game::IVec3 tile;
        if (!parse_ivec3(command.arguments, 0U, tile)) {
            return std::nullopt;
        }
        const auto internal = incoming(tile, spawn);
        game_.set_player_position({
            static_cast<double>(internal.x),
            static_cast<double>(internal.y),
            static_cast<double>(internal.z),
        });
        return std::nullopt;
    }

    if (command.name == "world.getBlock" || command.name == "world.getBlockWithData") {
        if (command.arguments.size() != 3U) {
            return failed_request();
        }
        game::IVec3 position;
        if (!parse_ivec3(command.arguments, 0U, position)) {
            return failed_request();
        }
        const auto internal = incoming(position, spawn);
        const int id = game_.block_type(internal.x, internal.y, internal.z);
        if (command.name == "world.getBlockWithData") {
            return std::to_string(id) + "," +
                   std::to_string(game_.block_data(internal.x, internal.y, internal.z));
        }
        return std::to_string(id);
    }

    if (command.name == "world.setBlock") {
        if (command.arguments.size() != 4U && command.arguments.size() != 5U) {
            return std::nullopt;
        }
        game::IVec3 position;
        int block_type = 0;
        int block_data = 0;
        if (!parse_ivec3(command.arguments, 0U, position) ||
            !parse_int(command.arguments[3], block_type) ||
            (command.arguments.size() == 5U && !parse_int(command.arguments[4], block_data))) {
            return std::nullopt;
        }
        const auto internal = incoming(position, spawn);
        game_.set_block(internal.x, internal.y, internal.z, block_type, block_data);
        return std::nullopt;
    }

    if (command.name == "world.setBlocks") {
        if (command.arguments.size() != 7U && command.arguments.size() != 8U) {
            return std::nullopt;
        }
        game::IVec3 begin;
        game::IVec3 end;
        int block_type = 0;
        int block_data = 0;
        if (!parse_ivec3(command.arguments, 0U, begin) ||
            !parse_ivec3(command.arguments, 3U, end) ||
            !parse_int(command.arguments[6], block_type) ||
            (command.arguments.size() == 8U && !parse_int(command.arguments[7], block_data))) {
            return std::nullopt;
        }
        begin = incoming(begin, spawn);
        end = incoming(end, spawn);
        game_.set_blocks(begin.x, begin.y, begin.z,
                         end.x, end.y, end.z,
                         block_type, block_data);
        return std::nullopt;
    }

    if (command.name == "world.getHeight") {
        if (command.arguments.size() != 2U) {
            return failed_request();
        }
        int x = 0;
        int z = 0;
        if (!parse_int(command.arguments[0], x) || !parse_int(command.arguments[1], z)) {
            return failed_request();
        }
        return std::to_string(game_.height_at(x + spawn.x, z + spawn.z) - spawn.y);
    }

    if (command.name == "world.checkpoint.save") {
        if (command.arguments.empty()) {
            game_.save_checkpoint();
        }
        return std::nullopt;
    }

    if (command.name == "world.checkpoint.restore") {
        if (command.arguments.empty()) {
            game_.restore_checkpoint();
        }
        return std::nullopt;
    }

    if (command.name == "world.setting" || command.name == "player.setting") {
        if (command.arguments.size() != 2U) {
            return std::nullopt;
        }
        int raw = 0;
        if (!parse_int(command.arguments[1], raw)) {
            return std::nullopt;
        }
        if (command.name == "world.setting") {
            game_.set_world_setting(command.arguments[0], raw != 0);
        } else {
            game_.set_player_setting(command.arguments[0], raw != 0);
        }
        return std::nullopt;
    }

    if (command.name == "chat.post") {
        if (command.arguments.size() == 1U) {
            game_.post_chat(command.arguments[0]);
        }
        return std::nullopt;
    }

    if (command.name == "camera.mode.setNormal") {
        if (command.arguments.size() <= 1U) {
            game_.set_camera_mode(game::CameraMode::Normal);
        }
        return std::nullopt;
    }

    if (command.name == "camera.mode.setThirdPerson" || command.name == "camera.mode.setFollow") {
        if (command.arguments.size() <= 1U) {
            game_.set_camera_mode(game::CameraMode::ThirdPerson);
        }
        return std::nullopt;
    }

    if (command.name == "camera.mode.setFixed") {
        if (command.arguments.empty()) {
            game_.set_camera_mode(game::CameraMode::Fixed);
        }
        return std::nullopt;
    }

    if (command.name == "camera.mode.setPos" || command.name == "camera.setPos") {
        if (command.arguments.size() != 3U) {
            return std::nullopt;
        }
        game::Vec3 position;
        if (!parse_vec3(command.arguments, 0U, position)) {
            return std::nullopt;
        }
        game_.set_camera_position(incoming(position, spawn));
        return std::nullopt;
    }

    if (command.name == "events.block.hits") {
        if (!command.arguments.empty()) {
            return failed_request();
        }
        const auto hits = game_.poll_block_hits();
        std::string response;
        for (const auto& hit : hits) {
            const auto position = outgoing(hit.position, spawn);
            if (!response.empty()) {
                response.push_back('|');
            }
            response += format_ivec3(position) + "," +
                        std::to_string(hit.face) + "," +
                        std::to_string(hit.entity_id);
        }
        return response;
    }

    if (command.name == "events.clear") {
        if (command.arguments.empty()) {
            game_.clear_events();
        }
        return std::nullopt;
    }

    // Present in the distributed Python/Java clients even though entities were
    // still marked TBD in MCPI-PROTOCOL 0.1. Phase 1 exposes the local player
    // as entity 0 so those clients can operate without a second entity system.
    if (command.name == "world.getPlayerIds") {
        return command.arguments.empty() ? std::optional<std::string>("0") : failed_request();
    }

    if (command.name == "entity.getPos" && command.arguments.size() == 1U) {
        return format_vec3(outgoing(game_.player_position(), spawn));
    }
    if (command.name == "entity.getTile" && command.arguments.size() == 1U) {
        const auto exact = game_.player_position();
        const game::IVec3 tile{
            static_cast<int>(std::floor(exact.x)),
            static_cast<int>(std::floor(exact.y)),
            static_cast<int>(std::floor(exact.z)),
        };
        return format_ivec3(outgoing(tile, spawn));
    }
    if (command.name == "entity.setPos" && command.arguments.size() == 4U) {
        game::Vec3 position;
        if (parse_vec3(command.arguments, 1U, position)) {
            game_.set_player_position(incoming(position, spawn));
        }
        return std::nullopt;
    }
    if (command.name == "entity.setTile" && command.arguments.size() == 4U) {
        game::IVec3 position;
        if (parse_ivec3(command.arguments, 1U, position)) {
            const auto internal = incoming(position, spawn);
            game_.set_player_position({
                static_cast<double>(internal.x),
                static_cast<double>(internal.y),
                static_cast<double>(internal.z),
            });
        }
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace mcpi::api
