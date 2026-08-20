# Original-parity status

Target: **Minecraft: Pi Edition 0.1.1 alpha**.

Statuses are evidence claims, not progress vibes:

- `unknown` — not enough original evidence yet;
- `confirmed` — relevant original behavior/layout is evidenced, but reconstruction is not fully matched;
- `partial` — a working reconstruction exists, but known parity gaps remain;
- `matched` — the acceptance test/reference comparison for the tracked claim passes.

No row is promoted to `matched` without a concrete evidence path and acceptance-test name. A project test can prove that our reconstruction is internally coherent without proving undocumented original behavior.

| # | Subsystem | Status | Evidence | Acceptance test |
|---:|---|---|---|---|
| 1 | Original world generation / `RandomLevelSource` | partial | `src/world/RandomLevelSource.*`; `tests/parity/reference/worldgen-structure.ref`; `docs/reverse-engineering/class-map.md` | `worldgen_parity` (boundary/determinism; original terrain still partial) |
| 2 | Lighting propagation | partial | `src/world/LightEngine.*`; `docs/reverse-engineering/levelchunk-layout.md` identifies packed sky/block-light layers and defaults | `lighting_parity` (skylight column/shadow, source attenuation/removal, cross-chunk blocklight; exact original tables/lateral skylight still partial) |
| 3 | Original block behavior | partial | `src/world/BlockBehavior.*`; GameState routes block writes through update/light hooks; protocol block IDs and confirmed light behavior | `block_behavior` + `lighting_parity` (registry/update contract reconstructed; exact original per-block tables/tick rules still partial) |
| 4 | Original world save format | partial | `src/storage/Nbt.*`, `PiLevelStorage.*`, `LegacyLevelStorage.*`, `StorageRouter.*`; Version-3 little-endian `level.dat` metadata fields reconstructed from evidence | `storage_parity` (metadata/header/router matched to tracked claims; original Pi chunk persistence remains unconfirmed) |
| 5 | Player physics | partial | `src/game/Entity.*`, `Player.*`, `Physics.*`; `GameState` delegates movement/position to the authoritative Player | `player_physics` + `game_state_player` (AABB collision, finite bounds, deterministic gravity/jump behavior; exact original movement constants/traces remain partial) |
| 6 | Inventory/hotbar/items | partial | `src/game/Inventory.*`; Player owns the single authoritative Inventory; legacy save v1 migration + v2 stack/data persistence | `inventory` + `game_state_player` (9-slot stack/data/selection contract; exact original Pi inventory persistence fields remain partial) |
| 7 | Entity system | partial | `src/game/EntityRegistry.*`; local Player is registered as stable entity ID 0; dispatcher performs real ID lookup instead of aliasing arbitrary IDs | `entity_system` (stable IDs, duplicate/missing-ID handling, player-ID visibility and poll/clear hit-event contract; broader original entity simulation/AI remains partial) |
| 8 | Networking/multiplayer | unknown | `src/network/Packet.*`; `src/network/NetworkHandler.*`; protocol ancestry is reconstructed, but Pi multiplayer reachability remains unestablished | `network_parity` (packet framing/dispatch contract only; original multiplayer reachability remains unknown) |
| 9 | Minecraft voxel renderer | partial | `src/client/ChunkMesh.*`; `src/client/LevelRenderer.*`; project-owned procedural texture fallback; software depth buffer | `chunk_mesh` + `phase2_parity_acceptance` (visible faces, shared-face culling, opaque/translucent split, metadata UV selection, light propagation; pixel parity remains unverified) |
| 10 | Original asset loader | partial | `src/assets/AssetSource.hpp`; `OriginalPiAssetSource.*`; `FallbackAssetSource.*`; repository ships no Mojang assets | `asset_source` (local reads, traversal rejection, missing-file fallback; exact original asset lookup hierarchy remains partial) |
| 11 | Audio | partial | `src/client/SoundEngine.*`; `SdlAudioMixer.*`; local WAV lookup with silence fallback and distance attenuation | `ui_audio` (event lookup failure is nonfatal and attenuation math is deterministic; original sound event table/mix remains unverified) |
| 12 | Original UI | partial | `src/client/Screen.*`; `HudRenderer.*`; reference coordinate system fixed at 848×480 | `ui_audio` (title/game/pause transitions, hotbar/crosshair anchors and chat lifetime; visual/pixel parity remains partial) |
| 13 | Camera API integration | partial | `src/client/Camera.*`; `GameState` camera mode/position/target state; renderer and raycast consume resolved camera pose | `camera` + `phase2_parity_acceptance` (normal/third-person/fixed, collision shortening and invalid-target fallback; exact original transform constants remain partial) |
| 14 | MCPI API bug-for-bug behavior | partial | `tests/parity/reference/api-transcripts.ref`; `docs/reverse-engineering/api-coordinate-translation.md`; explicit `CommandResult` response classes | `api_transcript_parity` (tracked transcript cases pass; exhaustive undocumented bugs remain partial) |
| 15 | Java API completeness | partial | independently written `clients/java/src/pi` surface, event/tool classes, Java 25 JAR build | `java_api_surface` + `java_api_smoke` (known class/method surface compiles and integrates; unknown original quirks remain partial) |
| 16 | Python API completeness | partial | real `mcpi==1.2.1` client and example fixtures; compatibility extension includes `world.getBlocks` | `python_api_compatibility` + `python_examples_smoke` (tracked modern client surface passes) |
| 17 | Original game loop/tick ordering | confirmed | `src/game/GameLoop.*`; startup/app callgraph anchors exist; exact original update order/timing remains incomplete | `game_loop` (deterministic reconstructed ordering; original timing trace still incomplete) |
| 18 | Reverse-engineering class map | partial | `docs/reverse-engineering/anchor-index.tsv`; `class-map.md`; LevelChunk and translator notes | `reverse_engineering_docs` |
| 19 | Differential original-vs-recompile harness | partial | `src/parity/ReferenceCase.*`; `ReferenceSuite.*`; `mcpi-parity` | `parity_framework` + `parity_reference` + `parity_cli` |
| 20 | Platform coverage | partial | `.github/workflows/build.yml`; `docs/platforms.md`; checked-in Linux ARM cross toolchains | `platform_contract` + native CI matrix + ARM cross-build smoke (runtime claims remain separate) |

## Promotion rule

When a task becomes verifiably complete, update the row in the same commit that adds or strengthens the supporting acceptance test. A green build alone is not sufficient evidence of original parity. Cross-compilation is build evidence only; it does not promote runtime behavior.
