#pragma once

#include "api/Command.hpp"
#include "api/CommandResult.hpp"
#include "game/GameApi.hpp"

#include <optional>
#include <string>

namespace mcpi::api {

class ApiDispatcher {
public:
    explicit ApiDispatcher(game::GameApi& game);

    [[nodiscard]] CommandResult dispatch_result(const Command& command) const {
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
