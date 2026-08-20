#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace mcpi::parity {

enum class Status {
    Unknown,
    Confirmed,
    Partial,
    Matched,
};

struct ReferenceCase {
    std::string subsystem;
    std::string name;
    std::string reference_build;
    std::string input;
    std::string expected;
};

struct Comparison {
    bool matched = false;
    std::string actual;
    std::string expected;
    std::string detail;
};

[[nodiscard]] Comparison compare_text(const ReferenceCase& reference, std::string actual);
[[nodiscard]] std::string stable_hex_digest(std::span<const std::uint8_t> bytes);

} // namespace mcpi::parity
