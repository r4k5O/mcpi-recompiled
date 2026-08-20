#include "parity/ReferenceSuite.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Options {
    std::string suite;
    std::filesystem::path references;
    std::optional<std::filesystem::path> json;
};

std::string json_escape(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20U) {
                    constexpr char hex[] = "0123456789abcdef";
                    const auto byte = static_cast<unsigned char>(ch);
                    escaped += "\\u00";
                    escaped += hex[(byte >> 4U) & 0x0fU];
                    escaped += hex[byte & 0x0fU];
                } else {
                    escaped += ch;
                }
        }
    }
    return escaped;
}

void print_usage(std::ostream& output) {
    output << "Usage: mcpi-parity --suite <name> --references <file-or-directory> "
              "[--json <report.json>]\n";
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        auto require_value = [&](std::string_view option) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string(option) + " requires a value");
            }
            return argv[++i];
        };

        if (argument == "--suite") {
            options.suite = require_value(argument);
        } else if (argument == "--references") {
            options.references = require_value(argument);
        } else if (argument == "--json") {
            options.json = std::filesystem::path(require_value(argument));
        } else if (argument == "--help" || argument == "-h") {
            print_usage(std::cout);
            std::exit(0);
        } else {
            throw std::runtime_error("unknown argument: " + std::string(argument));
        }
    }

    if (options.suite.empty()) {
        throw std::runtime_error("--suite is required");
    }
    if (options.references.empty()) {
        throw std::runtime_error("--references is required");
    }
    return options;
}

void write_json_report(const std::filesystem::path& path, std::string_view suite_name,
                       const std::vector<mcpi::parity::ReferenceCase>& cases) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("could not write JSON report: " + path.string());
    }

    output << "{\n"
           << "  \"suite\": \"" << json_escape(suite_name) << "\",\n"
           << "  \"loaded\": " << cases.size() << ",\n"
           << "  \"evaluated\": 0,\n"
           << "  \"matched\": 0,\n"
           << "  \"failed\": 0,\n"
           << "  \"cases\": [\n";

    for (std::size_t index = 0; index < cases.size(); ++index) {
        const auto& reference = cases[index];
        output << "    {"
               << "\"reference_build\": \"" << json_escape(reference.reference_build) << "\", "
               << "\"subsystem\": \"" << json_escape(reference.subsystem) << "\", "
               << "\"name\": \"" << json_escape(reference.name) << "\", "
               << "\"input\": \"" << json_escape(reference.input) << "\", "
               << "\"expected\": \"" << json_escape(reference.expected) << "\", "
               << "\"status\": \"pending\"}";
        if (index + 1U != cases.size()) {
            output << ',';
        }
        output << '\n';
    }

    output << "  ]\n}\n";
    if (!output) {
        throw std::runtime_error("failed while writing JSON report: " + path.string());
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse_options(argc, argv);
        const auto references = mcpi::parity::ReferenceSuite::load(options.references);
        const auto selected = references.cases(options.suite);

        std::cout << "Loaded " << selected.size() << " reference case";
        if (selected.size() != 1U) {
            std::cout << 's';
        }
        std::cout << " for suite " << options.suite << ".\n";
        for (const auto& reference : selected) {
            std::cout << "- " << reference.name << " [pending]\n";
        }

        if (options.json) {
            write_json_report(*options.json, options.suite, selected);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "mcpi-parity: " << error.what() << '\n';
        print_usage(std::cerr);
        return 2;
    }
}
