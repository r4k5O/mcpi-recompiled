#include "parity/ReferenceSuite.hpp"

#include <cassert>
#include <filesystem>
#include <stdexcept>

int main(int argc, char** argv) {
    assert(argc == 2);

    const auto suite = mcpi::parity::ReferenceSuite::load(std::filesystem::path(argv[1]));
    assert(suite.all().size() == 1U);

    const auto api = suite.cases("api");
    assert(api.size() == 1U);
    assert(api.front().reference_build == "mcpi-0.1.1");
    assert(api.front().subsystem == "api");
    assert(api.front().name == "world-get-block-origin");
    assert(api.front().input == "world.getBlock(0,0,0)");
    assert(api.front().expected == "1");

    assert(suite.cases("worldgen").empty());

    bool rejected_missing = false;
    try {
        (void)mcpi::parity::ReferenceSuite::load(std::filesystem::path(argv[1]).parent_path() / "missing.ref");
    } catch (const std::runtime_error&) {
        rejected_missing = true;
    }
    assert(rejected_missing);

    return 0;
}
