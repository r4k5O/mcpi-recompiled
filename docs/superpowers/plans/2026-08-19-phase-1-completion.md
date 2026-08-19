# Phase 1 Completion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the repository's documented Phase 1 compatibility surface with an evidence-driven LevelChunk, the full documented MCPI 0.1 protocol surface, Python + Java client compatibility, deterministic local world generation/persistence, and a minimal native playable client shell for Linux x86-64 and Windows x86-64.

**Architecture:** Keep historical compatibility behavior at explicit boundaries. `world::Chunk` models the confirmed 16×128×16 storage; `GameState` owns world/player/inventory/save state; `ApiDispatcher` performs spawn-relative MCPI translation and documented command dispatch; a platform client layer drives input/rendering without leaking platform APIs into world logic. Original assets/binary remain external.

**Tech Stack:** C++20, CMake, standard library, existing TCP server, Java JDK for compatibility tests, platform-native Win32/X11 software rendering for a dependency-light first client.

**Spec:** `docs/reconstruction.md` plus `docs/reverse-engineering/*.md`.

## Global Constraints

- Reference target is Minecraft: Pi Edition 0.1.1 alpha.
- Do not commit original Mojang executable, assets, or copied proprietary source/decompiler output.
- Linux x86-64 and Windows x86-64 must build in CI.
- MCPI server default port remains 4711.
- Gameplay modernization is out of scope until compatibility behavior is established.
- Reverse-engineered facts must remain distinguishable from provisional behavior.

---

### Task 1: Evidence-driven LevelChunk storage

**Files:** `src/world/Chunk.*`, `src/world/World.*`, `tests/chunk_storage_tests.cpp`

**Interfaces:** preserve `World::block_at` / `set_block`; add fixed-height chunk accessors for block ID/data and height.

- [ ] Add failing tests for 16×128×16 bounds, metadata nibbles, height map, and negative chunk routing.
- [ ] Verify RED in CI.
- [ ] Replace sparse per-block chunk map with fixed block IDs + packed nibble metadata + height map while retaining sparse chunk allocation at the World level.
- [ ] Verify existing Python/runtime tests remain green.

### Task 2: Original MCPI 0.1 compatibility surface

**Files:** `src/game/GameApi.hpp`, `src/game/GameState.*`, `src/api/ApiDispatcher.*`, `tests/api_dispatcher_tests.cpp`, `tests/python_api_smoke.py`

**Interfaces:** add `set_blocks`, `height`, tile positions, checkpoints, settings, camera state, hit events, and spawn-relative coordinate translation.

- [ ] Add failing dispatcher tests for all protocol commands documented in the supplied 0.1 specification.
- [ ] Implement spawn-relative incoming/outgoing translation using explicit spawn coordinates.
- [ ] Implement block range fill, height query, checkpoints, settings, camera operations, tile operations, and event clearing/polling.
- [ ] Extend real Python smoke test across the new commands.

### Task 3: Java compatibility client

**Files:** `clients/java/src/pi/*.java`, `tests/java_api_smoke.java`, `CMakeLists.txt`, `.github/workflows/build.yml`

**Interfaces:** public package `pi`, with core `Minecraft`, `Connection`, `Block`, `Vec`, `VecFloat` API compatible with the distributed Java source for the supported Phase-1 subset.

- [ ] Add Java smoke test that fails before client sources exist.
- [ ] Implement independent Java TCP client against port 4711.
- [ ] Test world/player/chat calls against the real `mcpi-recompiled` executable.
- [ ] Run on Linux and Windows CI using the runner JDK.

### Task 4: World generation and persistence

**Files:** `src/world/WorldGenerator.*`, `src/world/WorldSave.*`, `src/game/GameState.*`, `tests/world_persistence_tests.cpp`

**Interfaces:** deterministic `generate(seed)`, `save(path)`, `load(path)`; no original asset/save redistribution.

- [ ] Add failing generation/save/load tests.
- [ ] Implement a deterministic finite 256×128×256 Phase-1 terrain profile with grass/dirt/stone layers and spawn placement.
- [ ] Implement a compact versioned project save format preserving seed, spawn, player state, hotbar, and changed blocks.
- [ ] Verify save-load round trips and API state after reload.

### Task 5: Player, hotbar and interaction model

**Files:** `src/game/Player.*`, `src/game/Inventory.*`, `src/game/GameState.*`, `tests/player_gameplay_tests.cpp`

**Interfaces:** movement vectors, selected hotbar slot, place/break at target coordinate, finite-world collision/bounds.

- [ ] Add failing tests for movement, world bounds, slot selection, block placement and removal.
- [ ] Implement minimal compatibility gameplay state independent of rendering/input.
- [ ] Preserve API mutations and direct gameplay mutations on the same World instance.

### Task 6: Native playable shell

**Files:** `src/client/*`, `src/platform/*`, `src/main.cpp`, `CMakeLists.txt`, `.github/workflows/build.yml`

**Interfaces:** `ClientApp::run(GameState&)`; platform event/window backend; simple software voxel view.

- [ ] Add headless client-model tests and `--headless` API mode to keep CI noninteractive.
- [ ] Add Win32 and X11 window/input implementations using system APIs already available on target runners; no original textures are embedded.
- [ ] Render a simple colored block world, first-person camera/crosshair, hotbar, and menu/world selection sufficient to exercise Phase-1 gameplay.
- [ ] Wire keyboard/mouse movement, block break/place, hotbar selection, save/load, and API server into the same process.
- [ ] Keep `--headless` as the deterministic test/runtime mode used by CI.

### Task 7: Phase-1 acceptance gate

**Files:** `README.md`, `docs/reconstruction.md`, CI/test files

- [ ] Add a single `phase1_acceptance` CTest covering generation, persistence, API, player interaction and process startup.
- [ ] Run full Linux and Windows CI from a fresh commit.
- [ ] Update status/checklist with exact supported behavior and explicitly list remaining parity differences as post-Phase-1 reconstruction work rather than silently claiming pixel-perfect parity.
