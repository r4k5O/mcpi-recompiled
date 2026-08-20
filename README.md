# mcpi-recompiled

A source-reconstruction and native-port project for **Minecraft: Pi Edition 0.1.1 alpha**, focused on reproducing the original game and its programming API on modern systems.

> [!IMPORTANT]
> This project is unofficial and is not affiliated with Mojang Studios or Microsoft. **Minecraft: Pi Edition was officially released as a free download**; this project does not imply that the original game had to be purchased. The original game and its Mojang/Microsoft-owned executable, assets, trademarks, and other material are not covered by this repository's license and are not intentionally redistributed here.

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

## Phase 2 — evidence-driven original-parity reconstruction

Phase 2 deepens the reconstruction without converting unknown behavior into unsupported claims. The authoritative row-by-row status is [`docs/parity-status.md`](docs/parity-status.md).

Implemented Phase-2 infrastructure and reconstruction includes:

- differential/reference-vector parity tooling and reverse-engineering evidence contracts;
- deeper world generation, lighting, block-update, storage, inventory, physics, entity and game-loop layers;
- packet/network-handler boundaries while leaving original multiplayer reachability explicitly unresolved;
- MCPI transcript classification for response / no-response / `Fail` behavior;
- a broader independently written Java API surface packaged as `mcpi-java.jar`;
- real `mcpi==1.2.1` Python compatibility and example smoke tests;
- authoritative normal, third-person and fixed camera poses used by rendering and ray casting;
- a safe local asset-source abstraction with traversal rejection and project-owned fallback data;
- full visible voxel-face chunk meshes, metadata-dependent UV coordinates, light values, opaque/translucent separation, software depth testing, fog and selection outline;
- title/game/pause state, an 848×480-reference HUD layout, chat lifetime handling and local positional sound lookup;
- CI/build contracts for Linux, Windows, macOS and Linux ARM cross-build portability;
- a cross-subsystem Phase-2 acceptance gate plus a parity-report contract that rejects unsupported `matched` rows.

### What Phase 2 still does *not* claim

A working reconstruction is not automatically an original match. In particular, the project still does **not** claim exact original terrain output, every original lighting/block table, original chunk persistence, exact movement constants, full entity AI, original multiplayer reachability, pixel-identical rendering/UI, exact sound event/mixing tables, or bug-for-bug coverage of every undocumented API edge. Those remain `partial`, `confirmed`, or `unknown` until evidence and comparison tests justify promotion.

### Original Minecraft Pi assets

This repository does **not** redistribute Mojang/Microsoft textures, sounds or packaged game data. If you already have an original Minecraft: Pi Edition installation, point the desktop client at a local asset directory:

```bash
./mcpi-recompiled --assets /path/to/your/minecraft-pi/assets
```

The loader only reads local files, rejects path traversal, never downloads assets, and falls back to project-owned/procedural data when an optional asset is unavailable. `MCPI_ASSETS` can also name a local asset root. Supplying original assets does not make unresolved renderer/audio behavior automatically `matched`.

## Releases

The preferred release path is now entirely in GitHub:

1. open **Actions → Release → Run workflow**;
2. choose `patch`, `minor`, `major`, or `custom`;
3. for `custom`, enter a version such as `0.2.5` or `v0.2.5`;
4. run the workflow.

The workflow reads the newest `vMAJOR.MINOR.PATCH` tag, computes the requested next version, creates the annotated tag on `main`, builds and tests Linux x86-64 and Windows x86-64, packages both platforms, generates `SHA256SUMS.txt`, and publishes the GitHub Release. Release runs are serialized so two simultaneous manual releases cannot race for the same next version.

Manually pushed `v*` tags remain supported as an alternative. Release assets include `README.md`, `LICENSE`, `NOTICE`, and `LEGAL.md` alongside the executable. Release notes are generated automatically.

The release workflows deliberately use maintained GitHub Action majors. CI also installs the Linux SDL development dependencies needed by the enabled backends, and project warnings are fixed in source rather than hidden by weaker compiler flags.

The intended first public release is **`v0.1.0`**, representing the completed Phase-1 functional baseline. It is also the recommended fork point for projects that want to modernize gameplay, visuals, UI, or other behavior separately from this repository's original-parity work.

The Phase-1 baseline is licensed for **noncommercial** use under PolyForm Noncommercial 1.0.0 as described in [`LICENSE`](LICENSE) and [`LEGAL.md`](LEGAL.md).

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
--assets <path>       Local Minecraft Pi asset directory; never downloaded
--help                Show usage
```

Desktop controls:

- `N` — create a new world from the title screen
- `L` — load the configured world
- `Enter` — continue an already loaded world / resume from pause
- `WASD` — move
- mouse — look
- `Space` / `Ctrl` — move vertically in the current reconstructed movement model
- `Shift` — faster movement
- `1`–`9` — select hotbar slot
- left mouse — break targeted block
- right mouse — place selected block
- `F5` — save
- `F9` — reload while in game
- `Esc` — pause; from the title screen, quit
- `T` — from pause, return to the title screen

The MCPI API server runs in the same process, including while the desktop client is active.

## Build and test

```bash
cmake -S . -B build -DMCPI_BUILD_TESTS=ON -DMCPI_BUILD_CLIENT=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Tests cover protocol/TCP transport, dispatcher compatibility, LevelChunk layout, world lifecycle/save-load behavior, worldgen/lighting/block/storage reconstruction, player/inventory/entities/game-loop/network boundaries, transcript parity, camera transforms, asset safety, chunk meshes, UI/audio contracts, Python and Java clients, platform build contracts, the main executable, release/reverse-engineering documentation contracts, and integrated Phase-1/Phase-2 acceptance gates.

Additional native and cross-build recipes are documented in [`docs/platforms.md`](docs/platforms.md).

## Reconstruction approach

The compatibility target is **Minecraft: Pi Edition 0.1.1 alpha**. Reverse-engineering notes live in `docs/reverse-engineering/` and keep confirmed observations separate from inference.

The repository does not copy proprietary decompiler output. Observed layouts, constants, addresses, behavior, and compatibility tests are used to drive independently written source code.

Gameplay modernization remains separate from original-parity work.

## Repository policy

Do not commit original Mojang executables, textures, sounds, packaged game data, or leaked/proprietary source code. Keep reverse-engineering notes factual and distinguish confirmed observations from inference.

## License

Independently written project code and project-owned material are licensed under the **PolyForm Noncommercial License 1.0.0** unless a file or directory states otherwise. This is a source-available, noncommercial license rather than an OSI-approved open-source license.

See [`LICENSE`](LICENSE), [`NOTICE`](NOTICE), and [`LEGAL.md`](LEGAL.md) for the license reference, required notice, and scope. The license does **not** grant rights to Minecraft, Minecraft: Pi Edition, Mojang/Microsoft assets, trademarks, the original executable, or other third-party material.
