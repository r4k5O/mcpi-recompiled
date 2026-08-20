#include "assets/FallbackAssetSource.hpp"
#include "assets/OriginalPiAssetSource.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace {
void set_assets_environment(const std::optional<std::string>& value) {
#ifdef _WIN32
    if (value.has_value()) {
        _putenv_s("MCPI_ASSETS", value->c_str());
    } else {
        _putenv_s("MCPI_ASSETS", "");
    }
#else
    if (value.has_value()) {
        setenv("MCPI_ASSETS", value->c_str(), 1);
    } else {
        unsetenv("MCPI_ASSETS");
    }
#endif
}
}

int main() {
    namespace fs = std::filesystem;
    using mcpi::assets::FallbackAssetSource;
    using mcpi::assets::OriginalPiAssetSource;

    const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const fs::path root = fs::temp_directory_path() / ("mcpi-assets-" + std::to_string(nonce));
    fs::create_directories(root / "textures");

    const std::vector<unsigned char> expected{1, 2, 3, 4, 5};
    {
        std::ofstream out(root / "textures" / "terrain.bin", std::ios::binary);
        out.write(reinterpret_cast<const char*>(expected.data()), static_cast<std::streamsize>(expected.size()));
    }

    OriginalPiAssetSource source(root);
    const auto bytes = source.read("textures/terrain.bin");
    assert(bytes.has_value());
    assert(*bytes == expected);

    assert(!source.read("../secret.txt").has_value());
    assert(!source.read("textures/../../secret.txt").has_value());
    assert(!source.read(fs::path("/").string()).has_value());
    assert(!source.read("missing.bin").has_value());

    FallbackAssetSource fallback;
    const auto checker = fallback.read("fallback/checker.rgba");
    assert(checker.has_value());
    assert(checker->size() == 4U * 4U * 4U);
    assert(!fallback.read("missing/sound.ogg").has_value());

    // Mojang's Pi 0.1.1 archive extracts to a top-level directory named
    // "mcpi". Automatic discovery must therefore recognize ./mcpi without
    // requiring users to rename a legal original installation.
    const fs::path locator_sandbox = root / "locator-sandbox";
    const fs::path standard_install = locator_sandbox / "mcpi";
    fs::create_directories(standard_install / "data" / "images");

    const fs::path previous_cwd = fs::current_path();
    const char* raw_previous_assets = std::getenv("MCPI_ASSETS");
    const std::optional<std::string> previous_assets =
        raw_previous_assets == nullptr ? std::nullopt
                                       : std::optional<std::string>(raw_previous_assets);

    set_assets_environment(std::nullopt);
    fs::current_path(locator_sandbox);
    const auto located = OriginalPiAssetSource::locate(std::nullopt);
    fs::current_path(previous_cwd);
    set_assets_environment(previous_assets);

    assert(located.has_value());
    assert(*located == fs::weakly_canonical(standard_install));

    fs::remove_all(root);
    return 0;
}
