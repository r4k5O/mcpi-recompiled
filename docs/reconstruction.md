# Reconstruction notes

## Reference target

Initial target: **Minecraft: Pi Edition 0.1.1 alpha**.

The local reference archive inspected at project bootstrap contains the original Raspberry Pi release. The game executable is a 32-bit ARM Linux ELF binary, while the release also contains API material describing the MCPI protocol used by external programs.

## Rules for reconstruction

1. Do not commit the original game executable or packaged assets.
2. Do not use leaked or proprietary source code.
3. Record observations separately from assumptions.
4. Prefer behavior tests over guessed implementation details.
5. Keep platform adaptation separate from reconstructed game logic.
6. Do not modernize gameplay until compatibility work is complete.

## Phase 1 status — functional foundation complete

Phase 1 is the project's **functional compatibility foundation**, not a claim that every internal algorithm or visible frame already matches the original Pi Edition exactly.

### API acceptance

Implemented and tested:

- TCP server on port `4711` by default.
- Line-oriented MCPI command parsing and dispatch.
- Spawn-relative API coordinates based on the original `SpawnX/SpawnY/SpawnZ` behavior found during binary analysis.
- Player exact/tile positions.
- World get/set block, block data, inclusive `setBlocks`, and height queries.
- Checkpoint save/restore.
- World/player settings used by the distributed clients.
- Chat.
- Camera modes/position compatibility surface.
- Block-hit events and event clearing.
- Local-player entity compatibility surface.
- Real Python client smoke tests.
- Independent Java package-`pi` compatibility client and Java smoke tests.

### Game acceptance

Implemented and tested:

- Desktop startup and explicit headless startup.
- Minimal world menu/create/load flow.
- Deterministic finite 256×128×256 Phase-1 terrain.
- Persistent project save/load lifecycle.
- Evidence-driven 16×128×16 LevelChunk block layout with packed metadata/light storage and height map.
- Shared game/API world state.
- Player movement constrained to finite-world bounds.
- Nine-slot hotbar and selected block state.
- Block placement/removal and block-hit events.
- First-person mouse look and keyboard input.
- Asset-free block-world rendering in the original 848×480 window size.
- Save/reload controls in the client.
- Linux x86-64 and Windows x86-64 CI builds.

### Phase 1 automated gate

`phase1_acceptance` checks the major boundaries in one executable test:

1. LevelChunk dimensions/storage invariants;
2. deterministic generated world + spawn;
3. spawn-relative MCPI coordinates;
4. block ID + metadata operations and bulk fill;
5. checkpoint restore;
6. hotbar/direct gameplay mutation sharing the API world;
7. spawn-relative block-hit events;
8. save/load persistence of seed, spawn, hotbar, API changes, and gameplay changes.

The full CTest suite additionally runs Python, Java, TCP transport, dispatcher, chunk, world lifecycle, and main-executable tests.

## Remaining parity work after Phase 1

These are explicitly **not** treated as finished merely because the functional Phase-1 gate passes:

- exact original `RandomLevelSource` terrain generation;
- exact light propagation/update algorithms;
- complete original player physics/collision;
- full inventory/crafting/entity/AI/network behavior;
- exact menu/HUD/rendering behavior;
- original textures, sounds, fonts, and other proprietary assets;
- pixel-identical, binary-identical, or bug-for-bug parity.

Future reconstruction should replace provisional Phase-1 implementations with evidence-driven equivalents behind the same tests/interfaces rather than destabilizing the working foundation.

## Suggested reconstruction workflow

For each subsystem:

1. Document observable behavior in the original release.
2. Add a small compatibility test when the behavior can be automated.
3. Identify data structures/functions from binary analysis without importing proprietary code.
4. Write an independent C++ implementation.
5. Compare the port against the original behavior.
6. Mark unknowns explicitly instead of silently guessing.

## API-first strategy

The programming API gives the project a particularly useful black-box test surface. Commands such as world/block operations and player queries can be sent to the original release and the reconstructed server, allowing responses and resulting world state to be compared mechanically.

That API remains a regression anchor while deeper Level, terrain, lighting, player, entity, renderer and save-format behavior is reconstructed from the original reference.
