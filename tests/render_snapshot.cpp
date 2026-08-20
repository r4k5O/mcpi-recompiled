#include "client/LevelRenderer.hpp"
#include "game/GameState.hpp"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    mcpi::game::GameState game;
    game.world().clear();

    // Project-owned deterministic reconstruction scene. No original textures
    // or screenshots are embedded in this fixture.
    for (int x = 4; x <= 11; ++x) {
        for (int z = 4; z <= 13; ++z) {
            game.set_block(x, 0, z, 1, 0);
        }
    }
    game.set_block(7, 1, 8, 2, 0);
    game.set_block(8, 1, 8, 20, 0);
    game.set_block(9, 1, 8, 41, 0);
    game.set_block(8, 2, 10, 57, 0);

    mcpi::client::CameraPose camera;
    camera.position = {8.0, 6.0, -10.0};
    camera.yaw = 0.0;
    camera.pitch = -0.25;
    camera.target_entity_id = 0;

    mcpi::client::LevelRenderer renderer;
    const mcpi::world::BlockPos selected{9, 1, 8};
    const auto first = renderer.render(game.world(), camera, 320, 180, 2, selected);
    const auto second = renderer.render(game.world(), camera, 320, 180, 2, selected);

    assert(first.width == 320);
    assert(first.height == 180);
    assert(first.rgba.size() == static_cast<std::size_t>(320 * 180 * 4));
    assert(first.rgba == second.rgba);

    std::size_t changed_pixels = 0;
    for (std::size_t index = 4U; index < first.rgba.size(); index += 4U) {
        if (first.rgba[index + 0U] != first.rgba[0U] ||
            first.rgba[index + 1U] != first.rgba[1U] ||
            first.rgba[index + 2U] != first.rgba[2U]) {
            ++changed_pixels;
        }
    }
    assert(changed_pixels > 500U);

    const std::filesystem::path directory = "parity-artifacts";
    std::filesystem::create_directories(directory);
    const auto output = directory / "reconstruction.ppm";
    std::ofstream stream(output, std::ios::binary | std::ios::trunc);
    assert(stream.good());
    stream << "P6\n" << first.width << ' ' << first.height << "\n255\n";
    for (std::size_t index = 0; index < first.rgba.size(); index += 4U) {
        stream.put(static_cast<char>(first.rgba[index + 0U]));
        stream.put(static_cast<char>(first.rgba[index + 1U]));
        stream.put(static_cast<char>(first.rgba[index + 2U]));
    }
    stream.close();
    assert(std::filesystem::file_size(output) > 1000U);

    return 0;
}
