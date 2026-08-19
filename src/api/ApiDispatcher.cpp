#include "api/ApiDispatcher.hpp"

#include <cerrno>
#include <charconv>
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

std::optional<std::string> failed_request() {
    return std::string(kRequestFailed);
}

} // namespace

ApiDispatcher::ApiDispatcher(game::GameApi& game)
    : game_(game) {}

std::optional<std::string> ApiDispatcher::dispatch(const Command& command) const {
    if (command.name == "player.getPos") {
        if (!command.arguments.empty()) {
            return failed_request();
        }
        return format_vec3(game_.player_position());
    }

    if (command.name == "player.setPos") {
        if (command.arguments.size() != 3) {
            return std::nullopt;
        }

        game::Vec3 position;
        if (!parse_double(command.arguments[0], position.x) ||
            !parse_double(command.arguments[1], position.y) ||
            !parse_double(command.arguments[2], position.z)) {
            return std::nullopt;
        }

        game_.set_player_position(position);
        return std::nullopt;
    }

    if (command.name == "world.getBlock") {
        if (command.arguments.size() != 3) {
            return failed_request();
        }

        int x = 0;
        int y = 0;
        int z = 0;
        if (!parse_int(command.arguments[0], x) ||
            !parse_int(command.arguments[1], y) ||
            !parse_int(command.arguments[2], z)) {
            return failed_request();
        }

        return std::to_string(game_.block_type(x, y, z));
    }

    if (command.name == "world.setBlock") {
        if (command.arguments.size() != 4 && command.arguments.size() != 5) {
            return std::nullopt;
        }

        int x = 0;
        int y = 0;
        int z = 0;
        int block_type = 0;
        int block_data = 0;
        if (!parse_int(command.arguments[0], x) ||
            !parse_int(command.arguments[1], y) ||
            !parse_int(command.arguments[2], z) ||
            !parse_int(command.arguments[3], block_type)) {
            return std::nullopt;
        }
        if (command.arguments.size() == 5 && !parse_int(command.arguments[4], block_data)) {
            return std::nullopt;
        }

        game_.set_block(x, y, z, block_type, block_data);
        return std::nullopt;
    }

    if (command.name == "chat.post") {
        if (command.arguments.size() != 1) {
            return std::nullopt;
        }
        game_.post_chat(command.arguments[0]);
        return std::nullopt;
    }

    return std::nullopt;
}

} // namespace mcpi::api
