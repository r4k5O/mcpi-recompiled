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

    [[nodiscard]] CommandResult dispatch_result(const Command& command) const;
    [[nodiscard]] std::optional<std::string> dispatch(const Command& command) const;

private:
    game::GameApi& game_;
};

} // namespace mcpi::api
