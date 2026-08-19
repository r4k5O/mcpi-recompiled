#pragma once

#include "api/Command.hpp"
#include "game/GameApi.hpp"

#include <optional>
#include <string>

namespace mcpi::api {

class ApiDispatcher {
public:
    explicit ApiDispatcher(game::GameApi& game);

    [[nodiscard]] std::optional<std::string> dispatch(const Command& command) const;

private:
    game::GameApi& game_;
};

} // namespace mcpi::api
