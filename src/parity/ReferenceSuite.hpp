#pragma once

#include "parity/ReferenceCase.hpp"

#include <filesystem>
#include <string_view>
#include <vector>

namespace mcpi::parity {

class ReferenceSuite {
public:
    [[nodiscard]] static ReferenceSuite load(const std::filesystem::path& path);

    [[nodiscard]] const std::vector<ReferenceCase>& all() const noexcept;
    [[nodiscard]] std::vector<ReferenceCase> cases(std::string_view subsystem) const;

private:
    std::vector<ReferenceCase> cases_;
};

} // namespace mcpi::parity
