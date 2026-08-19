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
    {
        mcpi::world::Chunk chunk({2, -3});
        expect(chunk.empty(), "a new chunk should be empty");

        chunk.set_block({0, 7, 0}, {1, 0});
        chunk.set_block({15, 31, 15}, {35, 14});

        expect(chunk.block_at({0, 7, 0}) == mcpi::world::BlockState{1, 0},
               "chunk should store the first local corner");
        expect(chunk.block_at({15, 31, 15}) == mcpi::world::BlockState{35, 14},
               "chunk should store the opposite local corner including block data");
        expect(chunk.block_at({4, 12, 9}) == mcpi::world::BlockState{},
               "unset positions inside a chunk should read as air");

        chunk.set_block({0, 7, 0}, {});
        expect(chunk.block_at({0, 7, 0}) == mcpi::world::BlockState{},
               "setting air should remove a stored local block");
        expect(!chunk.empty(), "chunk should remain non-empty while another block exists");

        chunk.set_block({15, 31, 15}, {});
        expect(chunk.empty(), "chunk should become empty after its final block is removed");
    }

    {
        mcpi::world::World world;
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

        expect(world.block_at({0, 1, 0}) == mcpi::world::BlockState{1, 0},
               "chunk routing must preserve origin block state");
        expect(world.block_at({15, 2, 15}) == mcpi::world::BlockState{2, 0},
               "chunk routing must preserve positive edge block state");
        expect(world.block_at({16, 3, 0}) == mcpi::world::BlockState{3, 0},
               "chunk routing must preserve first block in the next chunk");
        expect(world.block_at({-1, 4, -1}) == mcpi::world::BlockState{4, 0},
               "chunk routing must preserve negative edge block state");
        expect(world.block_at({-16, 5, 0}) == mcpi::world::BlockState{5, 0},
               "chunk routing must preserve exact negative chunk boundary");
        expect(world.block_at({-17, 6, 0}) == mcpi::world::BlockState{6, 0},
               "chunk routing must preserve positions beyond a negative boundary");

        world.set_block({16, 3, 0}, {});
        expect(world.chunk_count() == 4,
               "an empty chunk should be released from the world");
    }

    std::cout << "Chunk storage tests passed.\n";
    return 0;
}
