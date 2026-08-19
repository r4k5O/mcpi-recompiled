#include "api/Command.hpp"

#include <iostream>
#include <string>

int main() {
    std::cout << "mcpi-recompiled bootstrap\n";
    std::cout << "Phase 1: Minecraft Pi 0.1.1 compatibility\n";
    std::cout << "API target: TCP 4711 / MCPI protocol\n";

    const auto sample = mcpi::api::parse_command("world.setBlock(1,2,3,1)");
    if (!sample) {
        std::cerr << "Internal protocol parser self-check failed.\n";
        return 1;
    }

    std::cout << "Parser ready: " << sample->name << " with "
              << sample->arguments.size() << " arguments\n";
    return 0;
}
