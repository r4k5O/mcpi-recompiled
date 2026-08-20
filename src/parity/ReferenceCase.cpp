#include "parity/ReferenceCase.hpp"

#include <iomanip>
#include <sstream>

namespace mcpi::parity {

Comparison compare_text(const ReferenceCase& reference, std::string actual) {
    Comparison result;
    result.matched = actual == reference.expected;
    result.actual = std::move(actual);
    result.expected = reference.expected;

    if (result.matched) {
        result.detail = "matched " + reference.subsystem + "/" + reference.name;
    } else {
        result.detail = "mismatch " + reference.subsystem + "/" + reference.name +
                        ": expected '" + result.expected + "' but got '" + result.actual + "'";
    }

    return result;
}

std::string stable_hex_digest(std::span<const std::uint8_t> bytes) {
    constexpr std::uint64_t offset_basis = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;

    std::uint64_t value = offset_basis;
    for (const std::uint8_t byte : bytes) {
        value ^= static_cast<std::uint64_t>(byte);
        value *= prime;
    }

    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(16) << value;
    return output.str();
}

} // namespace mcpi::parity
