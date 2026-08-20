#pragma once

#include "api/Command.hpp"
#include "api/CommandResult.hpp"
#include "game/GameApi.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <optional>
#include <string>

namespace mcpi::api {

class ApiDispatcher {
public:
    explicit ApiDispatcher(game::GameApi& game);

    [[nodiscard]] CommandResult dispatch_result(const Command& command) const {
        // mcpi==1.2.1 exposes world.getBlocks even though the original Pi
        // command surface we have confirmed so far is narrower. Keep this
        // compatibility extension isolated from the legacy parity dispatcher.
        if (command.name == "world.getBlocks") {
            if (command.arguments.size() != 6U) {
                return CommandResult::fail();
            }

            std::array<int, 6> values{};
            for (std::size_t index = 0; index < values.size(); ++index) {
                const auto& text = command.arguments[index];
                const char* begin = text.data();
                const char* end = begin + text.size();
                const auto parsed = std::from_chars(begin, end, values[index]);
                if (parsed.ec != std::errc{} || parsed.ptr != end) {
                    return CommandResult::fail();
                }
            }

            const auto spawn = game_.spawn_position();
            const int min_x = std::min(values[0], values[3]) + spawn.x;
            const int max_x = std::max(values[0], values[3]) + spawn.x;
            const int min_y = std::min(values[1], values[4]) + spawn.y;
            const int max_y = std::max(values[1], values[4]) + spawn.y;
            const int min_z = std::min(values[2], values[5]) + spawn.z;
            const int max_z = std::max(values[2], values[5]) + spawn.z;

            std::string response;
            for (int y = min_y; y <= max_y; ++y) {
                for (int z = min_z; z <= max_z; ++z) {
                    for (int x = min_x; x <= max_x; ++x) {
                        if (!response.empty()) {
                            response.push_back(',');
                        }
                        response += std::to_string(game_.block_type(x, y, z));
                    }
                }
            }
            return CommandResult::response_value(std::move(response));
        }

        const auto legacy = dispatch(command);
        if (!legacy.has_value()) {
            return CommandResult::no_response();
        }
        if (*legacy == "Fail") {
            return CommandResult::fail();
        }
        return CommandResult::response_value(*legacy);
    }

    [[nodiscard]] std::optional<std::string> dispatch(const Command& command) const;

private:
    game::GameApi& game_;
};

} // namespace mcpi::api
