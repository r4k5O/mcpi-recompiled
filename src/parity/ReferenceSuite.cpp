#include "parity/ReferenceSuite.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace mcpi::parity {
namespace {

struct PendingCase {
    ReferenceCase value;
    bool has_reference_build = false;
    bool has_subsystem = false;
    bool has_name = false;
    bool has_input = false;
    bool has_expected = false;

    [[nodiscard]] bool empty() const noexcept {
        return !has_reference_build && !has_subsystem && !has_name && !has_input && !has_expected;
    }

    [[nodiscard]] bool complete() const noexcept {
        return has_reference_build && has_subsystem && has_name && has_input && has_expected;
    }
};

[[noreturn]] void parse_error(const std::filesystem::path& path, std::size_t line,
                              const std::string& message) {
    throw std::runtime_error(path.string() + ":" + std::to_string(line) + ": " + message);
}

void finish_case(const std::filesystem::path& path, std::size_t line, PendingCase& pending,
                 std::vector<ReferenceCase>& output) {
    if (pending.empty()) {
        return;
    }
    if (!pending.complete()) {
        parse_error(path, line, "reference case is missing one or more required fields");
    }
    if (pending.value.reference_build.empty()) {
        parse_error(path, line, "reference_build must not be empty");
    }
    if (pending.value.subsystem.empty()) {
        parse_error(path, line, "subsystem must not be empty");
    }
    if (pending.value.name.empty()) {
        parse_error(path, line, "name must not be empty");
    }

    output.push_back(std::move(pending.value));
    pending = PendingCase{};
}

void assign_field(const std::filesystem::path& path, std::size_t line, PendingCase& pending,
                  const std::string& key, const std::string& value) {
    auto duplicate = [&]() {
        parse_error(path, line, "duplicate field: " + key);
    };

    if (key == "reference_build") {
        if (pending.has_reference_build) duplicate();
        pending.value.reference_build = value;
        pending.has_reference_build = true;
    } else if (key == "subsystem") {
        if (pending.has_subsystem) duplicate();
        pending.value.subsystem = value;
        pending.has_subsystem = true;
    } else if (key == "name") {
        if (pending.has_name) duplicate();
        pending.value.name = value;
        pending.has_name = true;
    } else if (key == "input") {
        if (pending.has_input) duplicate();
        pending.value.input = value;
        pending.has_input = true;
    } else if (key == "expected") {
        if (pending.has_expected) duplicate();
        pending.value.expected = value;
        pending.has_expected = true;
    } else {
        parse_error(path, line, "unknown field: " + key);
    }
}

std::vector<ReferenceCase> load_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("could not open parity reference file: " + path.string());
    }

    std::vector<ReferenceCase> result;
    PendingCase pending;
    std::string line_text;
    std::size_t line_number = 0;

    while (std::getline(input, line_text)) {
        ++line_number;
        if (!line_text.empty() && line_text.back() == '\r') {
            line_text.pop_back();
        }
        if (line_text.empty() || line_text.front() == '#') {
            continue;
        }
        if (line_text == "---") {
            finish_case(path, line_number, pending, result);
            continue;
        }

        const auto equals = line_text.find('=');
        if (equals == std::string::npos || equals == 0) {
            parse_error(path, line_number, "expected key=value or ---");
        }

        assign_field(path, line_number, pending, line_text.substr(0, equals),
                     line_text.substr(equals + 1));
    }

    finish_case(path, line_number + 1, pending, result);
    if (result.empty()) {
        throw std::runtime_error("parity reference file contains no cases: " + path.string());
    }
    return result;
}

} // namespace

ReferenceSuite ReferenceSuite::load(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("parity reference path does not exist: " + path.string());
    }

    ReferenceSuite suite;
    if (std::filesystem::is_regular_file(path)) {
        suite.cases_ = load_file(path);
        return suite;
    }

    if (!std::filesystem::is_directory(path)) {
        throw std::runtime_error("parity reference path is neither a file nor directory: " + path.string());
    }

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".ref") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    if (files.empty()) {
        throw std::runtime_error("parity reference directory contains no .ref files: " + path.string());
    }

    for (const auto& file : files) {
        auto loaded = load_file(file);
        suite.cases_.insert(suite.cases_.end(), std::make_move_iterator(loaded.begin()),
                            std::make_move_iterator(loaded.end()));
    }
    return suite;
}

const std::vector<ReferenceCase>& ReferenceSuite::all() const noexcept {
    return cases_;
}

std::vector<ReferenceCase> ReferenceSuite::cases(std::string_view subsystem) const {
    std::vector<ReferenceCase> filtered;
    for (const auto& reference : cases_) {
        if (reference.subsystem == subsystem) {
            filtered.push_back(reference);
        }
    }
    return filtered;
}

} // namespace mcpi::parity
