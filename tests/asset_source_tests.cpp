#include "assets/FallbackAssetSource.hpp"
#include "assets/OriginalPiAssetSource.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>

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

    fs::remove_all(root);
    return 0;
}
