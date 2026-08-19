#include "api/Command.hpp"

#include <cctype>

namespace mcpi::api {
namespace {

std::string trim(std::string_view value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return std::string(value.substr(begin, end - begin));
}

} // namespace

std::optional<Command> parse_command(std::string_view line) {
    const std::string cleaned = trim(line);
    if (cleaned.empty()) {
        return std::nullopt;
    }

    const auto open = cleaned.find('(');
    const auto close = cleaned.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close < open || close != cleaned.size() - 1) {
        return std::nullopt;
    }

    Command command;
    command.name = trim(std::string_view(cleaned).substr(0, open));
    if (command.name.empty()) {
        return std::nullopt;
    }

    const std::string_view body = std::string_view(cleaned).substr(open + 1, close - open - 1);
    std::size_t start = 0;
    while (start <= body.size()) {
        const auto comma = body.find(',', start);
        const auto end = comma == std::string_view::npos ? body.size() : comma;
        const auto arg = trim(body.substr(start, end - start));
        if (!arg.empty()) {
            command.arguments.push_back(arg);
        } else if (!body.empty()) {
            command.arguments.emplace_back();
        }

        if (comma == std::string_view::npos) {
            break;
        }
        start = comma + 1;
    }

    return command;
}

} // namespace mcpi::api
