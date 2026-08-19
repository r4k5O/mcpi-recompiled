#include "api/Command.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using mcpi::api::parse_command;

    {
        const auto command = parse_command("world.setBlock(1,2,3,57)\n");
        expect(command.has_value(), "world.setBlock should parse");
        expect(command->name == "world.setBlock", "command name should match");
        expect(command->arguments.size() == 4, "world.setBlock should expose four arguments");
        expect(command->arguments[3] == "57", "last argument should be preserved");
    }

    {
        const auto command = parse_command("player.getPos()\r\n");
        expect(command.has_value(), "zero-argument command should parse");
        expect(command->name == "player.getPos", "player.getPos name should match");
        expect(command->arguments.empty(), "zero-argument command should have no arguments");
    }

    {
        const auto command = parse_command("chat.post(Hello from Pi)");
        expect(command.has_value(), "chat.post should parse");
        expect(command->arguments.size() == 1, "chat.post should have one argument");
        expect(command->arguments[0] == "Hello from Pi", "text payload should be preserved");
    }

    expect(!parse_command("").has_value(), "empty input should be rejected");
    expect(!parse_command("not-a-command").has_value(), "malformed input should be rejected");
    expect(!parse_command("world.setBlock(1,2,3,1) trailing").has_value(), "trailing data should be rejected");

    std::cout << "All protocol parser tests passed.\n";
    return 0;
}
