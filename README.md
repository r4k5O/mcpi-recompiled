# mcpi-recompiled

A source-reconstruction and native-port project for **Minecraft: Pi Edition 0.1.1 alpha**, focused on reproducing the original game and its programming API on modern systems.

> [!IMPORTANT]
> This project is unofficial and is not affiliated with Mojang Studios or Microsoft. It does **not** redistribute the original `minecraft-pi` executable or game assets. Users must obtain any original files they are legally entitled to use themselves.

## Phase 1 — functional compatibility foundation ✅

Phase 1 establishes a runnable modern foundation rather than claiming pixel-perfect or bug-for-bug parity with the original game.

Implemented in Phase 1:

- native **Linux x86-64** and **Windows x86-64** builds;
- an SDL3 desktop client with the original 848×480 window size, first-person mouse look, keyboard movement, hotbar selection, block breaking/placing, a minimal menu, and save/load controls;
- explicit `--headless` mode for API-only/server-style use;
- a finite **256×128×256** world model;
- evidence-driven `LevelChunk` storage with 16×128×16 blocks, packed metadata, light-storage layers, and a 16×16 height map;
- deterministic seed-based Phase-1 terrain generation;
- persistent project save files containing seed, spawn, player state, hotbar state, and block overrides;
- shared world state between direct gameplay and the programming API;
- the MCPI text protocol on TCP port **4711** with spawn-relative coordinates;
- world block access/fills/height, player position/tile operations, chat, checkpoints, settings, camera operations, block-hit events, and the distributed entity/player-id compatibility surface;
- real Python compatibility tests using `mcpi`;
- an independently written Java compatibility client under package `pi`, tested with Java 25;
- an integrated `phase1_acceptance` test plus Linux/Windows CI.

### What Phase 1 does *not* claim

The following remain reconstruction/parity work rather than being silently treated as complete:

- exact original `RandomLevelSource` terrain generation;
- original textures, sounds, fonts, and other Mojang-owned assets;
- exact lighting propagation and rendering behavior;
- exact survival/creative physics, collision, entity AI, networking, and every original UI detail;
- binary-identical, pixel-identical, or bug-for-bug behavior.

Those items are now easier to tackle because Phase 1 provides a tested executable, game state, chunk model, save lifecycle, API clients, and reverse-engineering evidence base.

## Releases

Version tags matching `v*` are built and tested automatically on Linux x86-64 and Windows x86-64 before a GitHub Release is published. Release assets include platform archives and `SHA256SUMS.txt`; release notes are generated automatically.

The release workflows deliberately use maintained GitHub Action majors (`checkout@v7`, `setup-java@v5`, `upload-artifact@v7`, `download-artifact@v8`) and `softprops/action-gh-release@v3`. CI also installs the Linux SDL development dependencies needed by the enabled backends, and project warnings are fixed in source rather than hidden by weaker compiler flags.

The intended first public release is **`v0.1.0`**, representing the completed Phase-1 functional baseline. It is also the recommended fork point for projects that want to modernize gameplay, visuals, UI, or other behavior separately from this repository's original-parity work.

The project code license must be documented before that first public release is tagged.

## Run

A normal build starts the desktop client:

```bash
./mcpi-recompiled
```

Useful options:

```text
--port <0-65535>      MCPI API port (default: 4711)
--headless            Run without a window
--world <path>        World save path (default: world.mcpiworld)
--seed <uint32>       Seed used when creating a new world
--help                Show usage
```

Desktop controls in the Phase-1 client:

- `N` — create a new world from the menu
- `L` — load the configured world
- `Enter` — enter an already loaded world
- `WASD` — move
- mouse — look
- `Space` / `Ctrl` — move vertically in the current Phase-1 movement model
- `Shift` — faster movement
- `1`–`9` — select hotbar slot
- left mouse — break targeted block
- right mouse — place selected block
- `F5` — save
- `F9` — reload
- `Esc` — return to menu / exit

The MCPI API server runs in the same process, including while the desktop client is active.

## Build and test

```bash
cmake -S . -B build -DMCPI_BUILD_TESTS=ON -DMCPI_BUILD_CLIENT=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Tests include protocol parsing, TCP transport, dispatcher compatibility, LevelChunk layout, world lifecycle/save-load behavior, the integrated Phase-1 acceptance gate, a real Python client, a real Java client, the built main executable, and a workflow contract that guards the maintained GitHub Action majors used by CI/releases.

## Reconstruction approach

The initial compatibility target is **Minecraft: Pi Edition 0.1.1 alpha**. Reverse-engineering notes live in `docs/reverse-engineering/` and keep confirmed observations separate from inference.

The repository does not copy proprietary decompiler output. Observed layouts, constants, addresses, behavior, and compatibility tests are used to drive independently written source code.

## Next reconstruction work

With Phase 1's functional surface established, subsequent work can focus on deeper original parity:

1. reconstruct `Level -> ChunkSource -> LevelChunk` behavior in more detail;
2. reproduce original `RandomLevelSource` generation from binary evidence;
3. reconstruct light propagation and block-update behavior;
4. deepen player physics, collision, inventory and entity behavior;
5. improve rendering/UI fidelity without redistributing original assets;
6. extend additional modern platform targets only after the compatibility behavior is stable.

Gameplay modernization remains separate from original-parity work.

## Repository policy

Do not commit original Mojang executables, textures, sounds, packaged game data, or leaked/proprietary source code. Keep reverse-engineering notes factual and distinguish confirmed observations from inference.

## License

Licensing for newly written project code will be documented before the first public release. Original Minecraft Pi files remain subject to their own terms and are not part of this repository.
