#include "parity/ReferenceCase.hpp"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using namespace mcpi::parity;

    const ReferenceCase reference{
        "api",
        "world-get-block-origin",
        "mcpi-0.1.1",
        "world.getBlock(0,0,0)",
        "1",
    };

    const auto same = compare_text(reference, "1");
    assert(same.matched);
    assert(same.actual == "1");
    assert(same.expected == "1");

    const auto different = compare_text(reference, "0");
    assert(!different.matched);
    assert(different.actual == "0");
    assert(different.expected == "1");
    assert(!different.detail.empty());

    const std::array<std::uint8_t, 3> bytes{1, 2, 3};
    const auto digest = stable_hex_digest(bytes);
    assert(digest.size() == 16U);
    assert(digest == stable_hex_digest(bytes));

    return 0;
}
