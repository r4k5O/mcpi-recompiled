#include "api/ApiDispatcher.hpp"
#include "api/Command.hpp"
#include "game/GameState.hpp"
#include "parity/ReferenceSuite.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::vector<std::string> split_transcript(std::string_view text) {
    std::vector<std::string> commands;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto separator = text.find('|', start);
        const auto end = separator == std::string_view::npos ? text.size() : separator;
        commands.emplace_back(text.substr(start, end - start));
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1U;
    }
    return commands;
}

std::string encode(const mcpi::api::CommandResult& result) {
    using mcpi::api::CommandResultKind;
    switch (result.kind) {
    case CommandResultKind::Response:
        return "response:" + result.response;
    case CommandResultKind::NoResponse:
        return "no-response";
    case CommandResultKind::Fail:
        return "fail";
    }
    return "invalid";
}

} // namespace

int main(int argc, char** argv) {
    assert(argc == 2);

    const auto suite = mcpi::parity::ReferenceSuite::load(std::filesystem::path(argv[1]));
    const auto cases = suite.cases("api-transcript");
    assert(!cases.empty());

    for (const auto& reference : cases) {
        mcpi::game::GameState game;
        game.world().clear();
        game.set_spawn_position({0, 0, 0});
        game.set_player_position({0.0, 0.0, 0.0});

        if (reference.name == "spawn-offset") {
            game.set_spawn_position({10, 20, 30});
        }

        mcpi::api::ApiDispatcher dispatcher(game);
        mcpi::api::CommandResult result = mcpi::api::CommandResult::no_response();

        for (const auto& line : split_transcript(reference.input)) {
            const auto command = mcpi::api::parse_command(line);
            assert(command.has_value());
            result = dispatcher.dispatch_result(*command);
        }

        const std::string actual = encode(result);
        if (actual != reference.expected) {
            std::cerr << reference.name << ": expected '" << reference.expected
                      << "' but got '" << actual << "'\n";
            return 1;
        }

        if (reference.name == "chat-comma") {
            assert(game.chat_messages().size() == 1U);
            assert(game.chat_messages().front() == "Hello,Pi");
        } else if (reference.name == "camera-setpos") {
            const auto camera = game.camera_position();
            assert(camera.x == 1.0 && camera.y == 2.0 && camera.z == 3.0);
        } else if (reference.name == "setblocks-clamp") {
            assert(game.block_type(0, 0, 0) == 1);
            assert(game.block_type(1, 1, 1) == 1);
        }
    }

    return 0;
}
