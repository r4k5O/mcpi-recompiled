# Minecraft: Pi Edition 0.1.1 class map

This document is an evidence index for the supplied original `minecraft-pi` ARM ELF. It records class relationships and reconstruction targets without committing the executable, assets, or decompiler output.

## Confidence vocabulary

- **confirmed** — directly supported by preserved RTTI/typeinfo/vtable structure, literal strings, field accesses, distributed API source/spec, or observed control flow.
- **strong inference** — several observations agree, but a semantic role or exact call relationship is not yet directly proven.
- **open** — hypothesis only; it must not be treated as parity behavior until measured.

The tabular machine-readable addresses live in [`anchor-index.tsv`](anchor-index.tsv).

## World and terrain

Primary reconstruction chain:

`Level -> ChunkSource -> RandomLevelSource -> LevelChunk`

What is confirmed:

- `Level`, `ChunkSource`, `RandomLevelSource`, and `LevelChunk` all have preserved RTTI/typeinfo/vtable anchors in the original ELF.
- `RandomLevelSource` type metadata identifies the `ChunkSource` relationship.
- `LevelChunk` stores block IDs as a 16 × 128 × 16 byte array with index `(x << 11) | (z << 7) | y`.
- the `LevelChunk` object has independently mapped metadata/light/height structures documented in [`levelchunk-layout.md`](levelchunk-layout.md).

### RandomLevelSource chunk path

A function at `0x000b46fc` is a **strong inference** candidate for the `RandomLevelSource` chunk lookup/generation path. Its directly observed behavior is useful even before an exact source-level name is assigned:

1. it receives two integer values that are used as chunk X/Z coordinates and combines them into a cache key;
2. it searches a tree/map-like cache embedded in the `RandomLevelSource` object;
3. on cache miss, it computes a chunk-dependent 32-bit value using constants `0x07ebe2d5` and `0x14609048` in the form `0x07ebe2d5 * chunkZ + 0x14609048 * chunkX` before reinitializing an RNG-like state;
4. it requests exactly `32768` bytes for the block-ID buffer;
5. it requests `720` bytes for a `LevelChunk`-sized object and calls the already mapped constructor path at `0x000b0a34` with the block buffer and chunk coordinates;
6. it calls terrain-generation candidates at `0x000b3b54` and `0x000b32ec` using the same chunk coordinates/block buffer;
7. it finally invokes a `LevelChunk` virtual through vtable offset `0x10` before returning the chunk pointer.

The `RandomLevelSource` constructor candidate at `0x000b4424` writes vptr `0x00110598` and initializes several large RNG/noise-like subobjects. The exact seed expansion, noise algorithms, and semantic names of `0x000b3b54` / `0x000b32ec` remain **open** until traced further.

Strong inference still to prove through call tracing and reference vectors:

- which `Level` virtual/call path requests chunks from its active `ChunkSource`;
- the exact `Dimension` factory path that creates `RandomLevelSource` for a normal local world;
- the exact seed/RNG/noise sequence behind the observed chunk-generation path;
- which of `0x000b3b54` and `0x000b32ec` correspond to base terrain, surface replacement, and/or feature population.

Open questions include dormant Pocket Edition ancestry that may be present in the binary but unreachable in Pi Edition gameplay. Presence of a class or packet does not by itself prove runtime reachability.

## Coordinate translation / API

`IPosTranslator -> OffsetPosTranslator`

The original binary preserves both RTTI relationships. Existing black-box/API evidence shows spawn-relative translation between external MCPI coordinates and internal world coordinates. The current evidence and formulas are documented in [`api-coordinate-translation.md`](api-coordinate-translation.md).

This mapping must remain separate from internal `LevelChunk` Y indexing: external API coordinate bounds do not imply negative internal chunk Y.

## Persistence

`LevelStorage -> ExternalFileLevelStorage`

Both classes have preserved typeinfo/vtable anchors. Phase 2 will trace these anchors into file/NBT/chunk I/O before replacing the current project-specific `MCPI_RECOMPILED_WORLD 1` format. Exact original-world compatibility remains **open** until round-trip reference cases pass.

## Runtime/gameplay

Anchors currently mapped:

- `Minecraft`
- `Inventory`
- `Entity`

These are structural anchors only. Exact inventory stack semantics, movement physics, collision, entity ticking, and game-loop order remain **open** until their behavior is measured and tests exist.

## Rendering

Anchors currently mapped:

- `LevelRenderer`
- `EntityRenderer`

The vtable addresses are confirmed structural evidence. The precise split between chunk mesh generation, camera transforms, culling, fog, lighting, and entity drawing is still a **strong inference** target for call-graph work.

## Networking

Anchors currently mapped:

- `ClientSideNetworkHandler`
- `ServerSideNetworkHandler`

The binary also contains packet RTTI/names including chunk, movement, placement, and block-update packets. Whether every inherited MCPE network path is reachable in the shipped Pi Edition is **open** and must be distinguished from merely present code.

## Top-level application

The binary spells the application RTTI name `NinecraftApp`; that spelling is preserved intentionally. Startup allocates the app object, the candidate constructor writes the same vptr recorded in `anchor-index.tsv`, and the main loop calls through that object. The startup facts and addresses remain documented in [`mcpi-0.1.1-initial-map.md`](mcpi-0.1.1-initial-map.md).

## Phase-2 use of this map

This file is not a completion claim. It exists to give each parity task a stable binary anchor. A subsystem moves from structural mapping to behavioral parity only when the parity harness has reference vectors and a test demonstrates the expected result. Unknowns stay marked **strong inference** or **open** rather than being filled from memory or unrelated source trees.
