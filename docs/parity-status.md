# Original-parity status

Target: **Minecraft: Pi Edition 0.1.1 alpha**.

Statuses are evidence claims, not progress vibes:

- `unknown` — not enough original evidence yet;
- `confirmed` — relevant original behavior/layout is evidenced, but reconstruction is not fully matched;
- `partial` — a working reconstruction exists, but known parity gaps remain;
- `matched` — the acceptance test/reference comparison for the tracked claim passes.

No row is promoted to `matched` without a concrete evidence path and acceptance-test name.

| # | Subsystem | Status | Evidence | Acceptance test |
|---:|---|---|---|---|
| 1 | Original world generation / `RandomLevelSource` | partial | `src/world/RandomLevelSource.*`; `tests/parity/reference/worldgen-structure.ref`; `docs/reverse-engineering/class-map.md` | `worldgen_parity` (boundary/determinism; original terrain still partial) |
| 2 | Lighting propagation | partial | `src/world/LightEngine.*`; `docs/reverse-engineering/levelchunk-layout.md` identifies packed sky/block-light layers and defaults | `lighting_parity` (skylight column/shadow, source attenuation/removal, cross-chunk blocklight; exact original tables/lateral skylight still partial) |
| 3 | Original block behavior | partial | Phase-1 block set/get/data path plus protocol block IDs | `block_behavior` (planned) |
| 4 | Original world save format | confirmed | storage/LevelData/NBT anchors and spawn-field evidence in RE notes | `storage_parity` (planned) |
| 5 | Player physics | partial | Phase-1 movement exists but is direct-position movement | `player_physics` (planned) |
| 6 | Inventory/hotbar/items | partial | Phase-1 9-slot block hotbar in `src/game/GameState.hpp` | `inventory` (planned) |
| 7 | Entity system | partial | MCPI entity compatibility currently aliases to local player behavior | `entity_system` (planned) |
| 8 | Networking/multiplayer | unknown | network-handler/packet ancestry is known but Pi reachability is not yet established | `network_parity` (planned) |
| 9 | Minecraft voxel renderer | partial | SDL3 Phase-1 renderer is runnable but intentionally simplified | `chunk_mesh` + reference-scene checks (planned) |
| 10 | Original asset loader | unknown | repository intentionally ships no Mojang assets | `asset_source` (planned) |
| 11 | Audio | unknown | original dependency includes SDL audio paths; reconstructed sound layer absent | `ui_audio` (planned) |
| 12 | Original UI | partial | Phase-1 minimal menu/HUD exists; original layout parity not claimed | `ui_audio` + layout references (planned) |
| 13 | Camera API integration | partial | camera state exists in GameState/API but is not authoritative in renderer | `camera` (planned) |
| 14 | MCPI API bug-for-bug behavior | partial | `docs/reverse-engineering/api-coordinate-translation.md`; existing dispatcher tests | `api_transcript_parity` (planned) |
| 15 | Java API completeness | partial | independent `clients/java/src/pi` surface + Java 25 smoke test | `java_api_surface` (planned) |
| 16 | Python API completeness | partial | real `mcpi==1.2.1` smoke test | `python_api_compatibility` (planned) |
| 17 | Original game loop/tick ordering | confirmed | startup/app callgraph anchors exist; exact update order/timing remains incomplete | `game_loop` (planned) |
| 18 | Reverse-engineering class map | partial | `docs/reverse-engineering/anchor-index.tsv`; `class-map.md`; LevelChunk and translator notes | `reverse_engineering_docs` |
| 19 | Differential original-vs-recompile harness | partial | `src/parity/ReferenceCase.*`; `ReferenceSuite.*`; `mcpi-parity` | `parity_framework` + `parity_reference` + `parity_cli` |
| 20 | Platform coverage | partial | Linux x86-64 + Windows x86-64 CI/release are established | `platform_contract` (planned) |

## Promotion rule

When a task becomes verifiably complete, update the row in the same commit that adds or strengthens the supporting acceptance test. A green build alone is not sufficient evidence of original parity.
