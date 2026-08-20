#include "parity/ReferenceCase.hpp"
#include "world/RandomLevelSource.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

std::string fingerprint(const mcpi::world::Chunk& chunk) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(mcpi::world::Chunk::block_count * 2U + mcpi::world::Chunk::column_count);

    for (int x = 0; x < mcpi::world::Chunk::width; ++x) {
        for (int z = 0; z < mcpi::world::Chunk::depth; ++z) {
            for (int y = 0; y < mcpi::world::Chunk::height; ++y) {
                const auto block = chunk.block_at({x, y, z});
                bytes.push_back(static_cast<std::uint8_t>(block.id & 0xff));
                bytes.push_back(static_cast<std::uint8_t>(block.data & 0x0f));
            }
            bytes.push_back(static_cast<std::uint8_t>(chunk.height_at(x, z) & 0xff));
        }
    }

    return mcpi::parity::stable_hex_digest(bytes);
}

} // namespace

int main() {
    using mcpi::world::Chunk;
    using mcpi::world::ChunkPos;
    using mcpi::world::RandomLevelSource;

    static_assert(Chunk::width == 16);
    static_assert(Chunk::height == 128);
    static_assert(Chunk::depth == 16);
    static_assert(Chunk::block_count == 32768U);

    // Strong-inference binary observation from candidate 0x000b46fc:
    // low 32 bits of 0x07ebe2d5 * chunkZ + 0x14609048 * chunkX are formed
    // before an RNG-like state is reinitialized. These vectors intentionally
    // test only that confirmed coordinate component; world-seed expansion and
    // noise usage remain open and must not be guessed here.
    assert(RandomLevelSource::observed_chunk_coordinate_mix(0, 0) == 0x00000000U);
    assert(RandomLevelSource::observed_chunk_coordinate_mix(1, 0) == 0x14609048U);
    assert(RandomLevelSource::observed_chunk_coordinate_mix(0, 1) == 0x07ebe2d5U);
    assert(RandomLevelSource::observed_chunk_coordinate_mix(4, 7) == 0x88f574f3U);
    assert(RandomLevelSource::observed_chunk_coordinate_mix(-1, 0) == 0xeb9f6fb8U);
    assert(RandomLevelSource::observed_chunk_coordinate_mix(0, -1) == 0xf8141d2bU);
    assert(RandomLevelSource::observed_chunk_coordinate_mix(-2, 3) == 0xef0287efU);

    constexpr std::uint32_t seed = 0x12345678U;
    constexpr int chunk_x = 4;
    constexpr int chunk_z = 7;

    const RandomLevelSource source(seed);
    const auto first = source.generate_chunk(chunk_x, chunk_z);
    const auto second = source.generate_chunk(chunk_x, chunk_z);

    assert(first.position() == (ChunkPos{chunk_x, chunk_z}));
    assert(second.position() == first.position());
    assert(fingerprint(first) == fingerprint(second));

    const RandomLevelSource other_seed(seed + 1U);
    const auto changed_seed = other_seed.generate_chunk(chunk_x, chunk_z);
    assert(fingerprint(first) != fingerprint(changed_seed));

    // Chunk generation must be a pure function of seed and chunk coordinates.
    // Generating another chunk in between must not perturb the result.
    const RandomLevelSource ordered(seed);
    const auto ordered_a = ordered.generate_chunk(2, 3);
    const auto ordered_b = ordered.generate_chunk(9, 1);
    const auto ordered_a_again = ordered.generate_chunk(2, 3);
    (void)ordered_b;

    const RandomLevelSource fresh(seed);
    const auto fresh_a = fresh.generate_chunk(2, 3);

    assert(fingerprint(ordered_a) == fingerprint(ordered_a_again));
    assert(fingerprint(ordered_a) == fingerprint(fresh_a));

    return 0;
}
