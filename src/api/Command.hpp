#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mcpi::api {

struct Command {
    std::string name;
    std::vector<std::string> arguments;
};

std::optional<Command> parse_command(std::string_view line);

} // namespace mcpi::api
