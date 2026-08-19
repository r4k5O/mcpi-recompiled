#include "world/Chunk.hpp"
#include "world/World.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using mcpi::world::BlockState;
    using mcpi::world::Chunk;
    using mcpi::world::World;

    expect(Chunk::width == 16, "LevelChunk width should be 16");
    expect(Chunk::height == 128, "LevelChunk internal height should be 128");
    expect(Chunk::depth == 16, "LevelChunk depth should be 16");
    expect(Chunk::block_count == 32768, "LevelChunk should contain 32768 block positions");
    expect(Chunk::nibble_storage_size == 16384,
           "each packed nibble layer should use 16384 bytes");

    {
        Chunk chunk({2, -3});
        expect(chunk.empty(), "a new chunk should be empty");

        chunk.set_block({0, 0, 0}, {1, 0});
        chunk.set_block({15, 127, 15}, {35, 14});

        expect(chunk.block_at({0, 0, 0}) == BlockState{1, 0},
               "chunk should store the first internal corner");
        expect(chunk.block_at({15, 127, 15}) == BlockState{35, 14},
               "chunk should store the opposite internal corner including metadata");
        expect(chunk.block_at({4, 12, 9}) == BlockState{},
               "unset positions inside a chunk should read as air");
        expect(chunk.block_at({0, -1, 0}) == BlockState{},
               "negative internal Y should read as air");
        expect(chunk.block_at({0, 128, 0}) == BlockState{},
               "Y=128 should be outside the original LevelChunk");

        expect(chunk.height_at(0, 0) == 1,
               "height map should store one plus the highest occupied Y");
        expect(chunk.height_at(15, 15) == 128,
               "height map should represent a block at the top internal layer");

        chunk.set_block({15, 127, 15}, {35, 3});
        expect(chunk.block_at({15, 127, 15}).data == 3,
               "metadata should round-trip through packed nibble storage");

        chunk.set_block({15, 126, 15}, {1, 0});
        chunk.set_block({15, 127, 15}, {});
        expect(chunk.height_at(15, 15) == 127,
               "removing the highest block should recompute the column height");

        chunk.set_block({0, 0, 0}, {});
        chunk.set_block({15, 126, 15}, {});
        expect(chunk.empty(), "chunk should become empty after its final block is removed");
        expect(chunk.height_at(0, 0) == 0 && chunk.height_at(15, 15) == 0,
               "empty columns should have zero height");
    }

    {
        World world;
        expect(world.chunk_count() == 0,
               "a new world should have no allocated chunks");

        world.set_block({0, 1, 0}, {1, 0});
        world.set_block({15, 2, 15}, {2, 0});
        expect(world.chunk_count() == 1,
               "world positions 0..15 on X/Z should share chunk 0,0");

        world.set_block({16, 3, 0}, {3, 0});
        expect(world.chunk_count() == 2,
               "x=16 should allocate the next positive chunk");

        world.set_block({-1, 4, -1}, {4, 0});
        expect(world.chunk_count() == 3,
               "-1,-1 should map to chunk -1,-1 rather than chunk 0,0");

        world.set_block({-16, 5, 0}, {5, 0});
        expect(world.chunk_count() == 4,
               "x=-16 should remain in chunk -1");

        world.set_block({-17, 6, 0}, {6, 0});
        expect(world.chunk_count() == 5,
               "x=-17 should floor-divide into chunk -2");

        expect(world.block_at({0, 1, 0}) == BlockState{1, 0},
               "chunk routing must preserve origin block state");
        expect(world.block_at({15, 2, 15}) == BlockState{2, 0},
               "chunk routing must preserve positive edge block state");
        expect(world.block_at({16, 3, 0}) == BlockState{3, 0},
               "chunk routing must preserve first block in the next chunk");
        expect(world.block_at({-1, 4, -1}) == BlockState{4, 0},
               "chunk routing must preserve negative edge block state");
        expect(world.block_at({-16, 5, 0}) == BlockState{5, 0},
               "chunk routing must preserve exact negative chunk boundary");
        expect(world.block_at({-17, 6, 0}) == BlockState{6, 0},
               "chunk routing must preserve positions beyond a negative boundary");

        expect(world.height_at(15, 15) == 3,
               "World::height_at should use the underlying chunk height map");

        world.set_block({16, 3, 0}, {});
        expect(world.chunk_count() == 4,
               "an empty chunk should be released from the world");

        const auto before = world.chunk_count();
        world.set_block({0, -1, 0}, {1, 0});
        world.set_block({0, 128, 0}, {1, 0});
        expect(world.chunk_count() == before,
               "out-of-height writes must not allocate chunks");
    }

    std::cout << "Chunk storage tests passed.\n";
    return 0;
}
