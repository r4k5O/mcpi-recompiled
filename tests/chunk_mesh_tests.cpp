#include "client/ChunkMesh.hpp"
#include "world/World.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace {
bool near(float a, float b, float epsilon = 1.0e-5f) {
    return std::fabs(a - b) <= epsilon;
}
}

int main() {
    using mcpi::client::ChunkMeshBuilder;
    using mcpi::world::BlockPos;
    using mcpi::world::BlockState;
    using mcpi::world::World;

    ChunkMeshBuilder builder;

    World isolated;
    isolated.set_block({0, 1, 0}, {1, 0});
    isolated.set_sky_light({0, 1, 0}, 15);
    auto mesh = builder.build(isolated, 0, 0);
    assert(mesh.opaque.size() == 36U);
    assert(mesh.translucent.empty());
    assert(std::all_of(mesh.opaque.begin(), mesh.opaque.end(), [](const auto& vertex) {
        return near(vertex.light, 1.0f);
    }));

    isolated.set_block({1, 1, 0}, {1, 0});
    mesh = builder.build(isolated, 0, 0);
    assert(mesh.opaque.size() == 60U);

    World translucent;
    translucent.set_block({4, 1, 0}, {8, 0});
    translucent.set_sky_light({4, 1, 0}, 15);
    mesh = builder.build(translucent, 0, 0);
    assert(mesh.opaque.empty());
    assert(mesh.translucent.size() == 36U);

    World metadata_zero;
    metadata_zero.set_block({2, 1, 2}, {5, 0});
    metadata_zero.set_sky_light({2, 1, 2}, 15);
    const auto zero_mesh = builder.build(metadata_zero, 0, 0);

    World metadata_one;
    metadata_one.set_block({2, 1, 2}, {5, 1});
    metadata_one.set_sky_light({2, 1, 2}, 15);
    const auto one_mesh = builder.build(metadata_one, 0, 0);
    assert(!zero_mesh.opaque.empty() && !one_mesh.opaque.empty());
    assert(!near(zero_mesh.opaque.front().u, one_mesh.opaque.front().u));

    World lit;
    lit.set_block({3, 1, 3}, {1, 0});
    lit.set_sky_light({3, 1, 3}, 0);
    lit.set_block_light({3, 1, 3}, 9);
    const auto lit_mesh = builder.build(lit, 0, 0);
    assert(!lit_mesh.opaque.empty());
    assert(near(lit_mesh.opaque.front().light, 9.0f / 15.0f));

    return 0;
}
