#pragma once

#include <optional>
#include <string>
#include <utility>

namespace mcpi::api {

enum class CommandResultKind {
    Response,
    NoResponse,
    Fail,
};

struct CommandResult {
    CommandResultKind kind = CommandResultKind::NoResponse;
    std::string response;

    [[nodiscard]] static CommandResult response_value(std::string value) {
        return {CommandResultKind::Response, std::move(value)};
    }

    [[nodiscard]] static CommandResult no_response() {
        return {CommandResultKind::NoResponse, {}};
    }

    [[nodiscard]] static CommandResult fail() {
        return {CommandResultKind::Fail, {}};
    }

    [[nodiscard]] std::optional<std::string> wire_response() const {
        switch (kind) {
        case CommandResultKind::Response:
            return response;
        case CommandResultKind::Fail:
            return std::string("Fail");
        case CommandResultKind::NoResponse:
            return std::nullopt;
        }
        return std::nullopt;
    }
};

} // namespace mcpi::api
