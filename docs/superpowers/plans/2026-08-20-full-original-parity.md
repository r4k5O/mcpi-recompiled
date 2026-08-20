# Full Original Parity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the Phase-1 compatibility placeholders with evidence-backed Minecraft: Pi Edition 0.1.1 parity across the 20 tracked reconstruction areas while keeping one reviewable Phase-2 pull request.

**Architecture:** A safe differential/oracle layer is built first, then world fidelity, simulation, API/networking, presentation, and platform support are implemented against that oracle. Each subsystem has an explicit interface and acceptance test; the original executable/assets stay local reference inputs and are never committed.

**Tech Stack:** C++20, CMake 3.20+, SDL3 3.4.14, Python test tooling, Java 25, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-08-20-full-original-parity-design.md`

## Global Constraints

- C++20 remains the language baseline.
- Linux x86-64 and Windows x86-64 stay green throughout Phase 2.
- Java compatibility compiles/tests on Java 25.
- MCPI API default port remains 4711.
- Internal finite world remains 256×128×256 unless stronger original evidence disproves it.
- No original Mojang executable/assets, copied decompiler output, or leaked source enters the repo.
- Original assets are optional user-provided inputs; project-owned fallbacks keep the binary runnable.
- Every behavioral change uses RED → GREEN where practical.
- `docs/parity-status.md` changes only when supporting evidence/test coverage exists.
- All work lands in the single `phase2/full-original-parity` pull request.

---

### Task 1: Parity status registry and reference-case core

**Files:**
- Create: `src/parity/ReferenceCase.hpp`
- Create: `src/parity/ReferenceCase.cpp`
- Create: `tests/parity_framework_tests.cpp`
- Create: `docs/parity-status.md`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `mcpi::parity::Status`, `ReferenceCase`, `Comparison`, `compare_text`, `stable_hex_digest`.
- Produces: canonical 20-row parity status document.

- [ ] **Step 1: Write the failing parity-framework test**

```cpp
#include "parity/ReferenceCase.hpp"
#include <array>
#include <cassert>

int main() {
    using namespace mcpi::parity;
    const ReferenceCase c{"api", "get-block", "mcpi-0.1.1", "world.getBlock(0,0,0)", "1"};
    const auto same = compare_text(c, "1");
    assert(same.matched);
    const auto different = compare_text(c, "0");
    assert(!different.matched);
    const std::array<std::uint8_t, 3> bytes{1, 2, 3};
    assert(stable_hex_digest(bytes) == stable_hex_digest(bytes));
}
```

- [ ] **Step 2: Register and run the test to verify RED**

Run: `cmake -S . -B build -DMCPI_BUILD_TESTS=ON -DMCPI_BUILD_CLIENT=OFF && cmake --build build --parallel && ctest --test-dir build -R parity_framework --output-on-failure`

Expected: compile failure because `parity/ReferenceCase.hpp` does not exist.

- [ ] **Step 3: Implement the minimal parity core**

`ReferenceCase.hpp` defines exactly:

```cpp
namespace mcpi::parity {
enum class Status { Unknown, Confirmed, Partial, Matched };
struct ReferenceCase { std::string subsystem, name, reference_build, input, expected; };
struct Comparison { bool matched; std::string actual, expected, detail; };
Comparison compare_text(const ReferenceCase&, std::string actual);
std::string stable_hex_digest(std::span<const std::uint8_t> bytes);
}
```

Use an independently implemented deterministic 64-bit FNV-1a digest rendered as 16 lowercase hex digits; this is a comparison fingerprint, not a cryptographic authenticity claim.

- [ ] **Step 4: Add `docs/parity-status.md` with all 20 items initially `partial`, `confirmed`, or `unknown` according to existing evidence**

The document must include columns: `#`, `Subsystem`, `Status`, `Evidence`, `Acceptance test` and must not mark any Phase-2-only subsystem `matched` before its test exists.

- [ ] **Step 5: Run all tests and commit**

Run: `ctest --test-dir build --output-on-failure`

Commit: `test: add parity reference framework`

### Task 2: Safe reference-vector loader and parity CLI

**Files:**
- Create: `src/parity/ReferenceSuite.hpp`
- Create: `src/parity/ReferenceSuite.cpp`
- Create: `src/parity/main.cpp`
- Create: `tests/parity/reference/api-basic.ref`
- Create: `tests/parity_reference_tests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `ReferenceCase` from Task 1.
- Produces: `ReferenceSuite::load(path)`, `ReferenceSuite::cases(subsystem)`, executable `mcpi-parity`.

- [ ] **Step 1: Add a failing parser test** using a line-oriented format:

```text
reference_build=mcpi-0.1.1
subsystem=api
name=world-get-block-origin
input=world.getBlock(0,0,0)
expected=1
---
```

The test asserts one case is loaded and filtered by `api`.

- [ ] **Step 2: Run RED**

Expected: missing `ReferenceSuite` symbols.

- [ ] **Step 3: Implement strict parser and CLI**

`mcpi-parity --suite api --references tests/parity/reference` loads all `.ref` files, prints each case name, and exits non-zero for malformed reference data. The CLI does not launch the original binary.

- [ ] **Step 4: Add JSON output mode**

`--json <path>` writes `{ "suite": "api", "cases": [...], "matched": N, "failed": N }` using a small hand-written JSON emitter with escaped strings; do not add a dependency.

- [ ] **Step 5: GREEN + commit**

Commit: `feat: add safe parity reference runner`

### Task 3: Reverse-engineering anchor index

**Files:**
- Create: `docs/reverse-engineering/anchor-index.tsv`
- Create: `docs/reverse-engineering/class-map.md`
- Create: `tests/reverse_engineering_docs_tests.py`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces machine-readable columns: `subsystem`, `symbol`, `address`, `evidence`, `confidence`, `summary`.

- [ ] **Step 1: Write RED doc-contract test** requiring anchors for `NinecraftApp`, `Level`, `LevelChunk`, `RandomLevelSource`, `IPosTranslator`, `OffsetPosTranslator`, storage, renderer, network, inventory, and entity families.
- [ ] **Step 2: Run RED** and confirm missing files fail.
- [ ] **Step 3: Populate known anchors from existing RE notes** without copying decompiler output. Unknown addresses use an empty field, not fabricated values; confidence is `confirmed`, `strong-inference`, or `open`.
- [ ] **Step 4: Add narrative callgraph/class-map links to existing `levelchunk-layout.md` and `api-coordinate-translation.md`**.
- [ ] **Step 5: GREEN + commit** `docs: add Phase 2 reverse engineering anchor index`.

### Task 4: RandomLevelSource chunk-generation boundary

**Files:**
- Create: `src/world/ChunkSource.hpp`
- Create: `src/world/RandomLevelSource.hpp`
- Create: `src/world/RandomLevelSource.cpp`
- Create: `tests/worldgen_parity_tests.cpp`
- Modify: `src/world/WorldGenerator.cpp`
- Modify: `src/world/WorldGenerator.hpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces:

```cpp
class ChunkSource {
public:
    virtual ~ChunkSource() = default;
    virtual Chunk generate_chunk(int chunk_x, int chunk_z) const = 0;
};
class RandomLevelSource final : public ChunkSource {
public:
    explicit RandomLevelSource(std::uint32_t seed);
    Chunk generate_chunk(int chunk_x, int chunk_z) const override;
};
```

- [ ] **Step 1: RED test** asserts deterministic identical chunk fingerprints for same seed/chunk, different fingerprints for selected differing seeds, correct 16×128×16 storage, and no dependence on generation order.
- [ ] **Step 2: Run RED** because `RandomLevelSource` is absent.
- [ ] **Step 3: Move Phase-1 generation behind `ChunkSource` without changing current generated output**; this creates the replacement seam first.
- [ ] **Step 4: Add committed safe original-derived worldgen reference cases for every original observation already documented; compare block/height fingerprints rather than proprietary data blobs**.
- [ ] **Step 5: Replace each Phase-1 terrain rule only when a reference vector/RE observation supports the replacement; unmatched regions remain explicitly `partial` in `parity-status.md`**.
- [ ] **Step 6: GREEN + commit** `refactor: introduce RandomLevelSource parity boundary`.

### Task 5: Light engine

**Files:**
- Create: `src/world/LightEngine.hpp`
- Create: `src/world/LightEngine.cpp`
- Create: `tests/lighting_parity_tests.cpp`
- Modify: `src/world/Chunk.hpp`
- Modify: `src/world/World.hpp`
- Modify: `CMakeLists.txt`

**Interfaces:**

```cpp
class LightEngine {
public:
    void rebuild(World& world) const;
    void on_block_changed(World& world, const BlockPos&, BlockState before, BlockState after) const;
};
```

- [ ] **Step 1: RED tests** cover full skylight column, opaque-block shadow, emissive source attenuation, source removal, and cross-chunk propagation.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement nibble-backed sky/block light propagation with bounded queues and block opacity/emission lookup**.
- [ ] **Step 4: Make block writes notify LightEngine through GameState rather than mutating light arrays directly**.
- [ ] **Step 5: Compare committed light reference vectors and update item 2 status**.
- [ ] **Step 6: GREEN + commit** `feat: add reconstructed light propagation`.

### Task 6: Block behavior registry

**Files:**
- Create: `src/world/BlockBehavior.hpp`
- Create: `src/world/BlockBehavior.cpp`
- Create: `tests/block_behavior_tests.cpp`
- Modify: `src/game/GameState.cpp`
- Modify: `src/world/World.cpp`

**Interfaces:**

```cpp
struct BlockBehavior {
    int opacity;
    int emission;
    bool solid;
    bool replaceable;
    bool scheduled_tick;
};
class BlockBehaviorRegistry {
public:
    static const BlockBehavior& behavior(int block_id);
};
```

- [ ] **Step 1: RED tests** for metadata preservation, solid/replaceable behavior, emissive blocks, neighbor notification count, and bounded scheduled tick queue.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement registry entries for block IDs used by confirmed Pi/API behavior; unknown IDs get conservative inert defaults**.
- [ ] **Step 4: Route place/break/setBlock changes through neighbor/light update hooks**.
- [ ] **Step 5: Add original-derived metadata/update cases and commit** `feat: add block behavior and update hooks`.

### Task 7: NBT and Pi level storage

**Files:**
- Create: `src/storage/Nbt.hpp`
- Create: `src/storage/Nbt.cpp`
- Create: `src/storage/LevelStorage.hpp`
- Create: `src/storage/PiLevelStorage.hpp`
- Create: `src/storage/PiLevelStorage.cpp`
- Create: `tests/storage_parity_tests.cpp`
- Modify: `src/game/GameState.hpp`
- Modify: `src/game/GameState.cpp`
- Modify: `src/main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**

```cpp
class LevelStorage {
public:
    virtual ~LevelStorage() = default;
    virtual bool load(game::GameState&, const std::filesystem::path&) = 0;
    virtual bool save(const game::GameState&, const std::filesystem::path&) = 0;
};
```

- [ ] **Step 1: RED tests** for NBT byte/string/int/list/compound round-trip, malformed-input rejection, and LevelData fields `SpawnX`, `SpawnY`, `SpawnZ`, seed, player position.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement bounded NBT reader/writer with explicit endian conversion and allocation limits**.
- [ ] **Step 4: Implement `PiLevelStorage` using only confirmed original field/container layout; keep legacy `.mcpiworld` reader as `LegacyLevelStorage` migration path**.
- [ ] **Step 5: Add CLI storage selection by path/format detection without changing default usability**.
- [ ] **Step 6: GREEN + commit** `feat: add reconstructed Pi level storage`.

### Task 8: Inventory and item stacks

**Files:**
- Create: `src/game/Inventory.hpp`
- Create: `src/game/Inventory.cpp`
- Create: `tests/inventory_tests.cpp`
- Modify: `src/game/GameState.hpp`
- Modify: `src/game/GameState.cpp`

**Interfaces:**

```cpp
struct ItemStack { int item_id = 0; int count = 0; int data = 0; };
class Inventory {
public:
    static constexpr int hotbar_size = 9;
    ItemStack& slot(int);
    const ItemStack& slot(int) const;
    int selected_slot() const noexcept;
    void select(int) noexcept;
};
```

- [ ] **Step 1: RED tests** for slot bounds, selection clamp, stack/data persistence, selected item placement.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Replace raw `std::array<int,9>` hotbar with `Inventory`; keep compatibility getters temporarily delegating to it**.
- [ ] **Step 4: Persist inventory through both legacy and Pi storage paths where evidence supports fields**.
- [ ] **Step 5: GREEN + commit** `feat: reconstruct inventory item stacks`.

### Task 9: Entity, player, collision and physics

**Files:**
- Create: `src/game/Entity.hpp`
- Create: `src/game/Entity.cpp`
- Create: `src/game/Player.hpp`
- Create: `src/game/Player.cpp`
- Create: `src/game/Physics.hpp`
- Create: `src/game/Physics.cpp`
- Create: `tests/player_physics_tests.cpp`
- Modify: `src/game/GameState.hpp`
- Modify: `src/game/GameState.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**

```cpp
struct Aabb { Vec3 min, max; };
struct Velocity { double x = 0, y = 0, z = 0; };
class Entity { /* id, position, velocity, bounds, tick */ };
class Player final : public Entity { /* inventory, on_ground */ };
class Physics { public: static void move(World&, Entity&, const Vec3& desired_delta); };
```

- [ ] **Step 1: RED tests** for floor collision, wall collision, ceiling collision, gravity trace, jump landing, finite-world edge, and non-penetration after repeated ticks.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement axis-separated AABB voxel collision with deterministic ordering**.
- [ ] **Step 4: Move player position ownership from raw `GameState::player_position_` to `Player`; API getters/setters delegate to the player**.
- [ ] **Step 5: Add evidence-derived movement trace fixtures and update item 5 status**.
- [ ] **Step 6: GREEN + commit** `feat: add player entity and collision physics`.

### Task 10: Entity registry and block-hit event source

**Files:**
- Create: `src/game/EntityRegistry.hpp`
- Create: `src/game/EntityRegistry.cpp`
- Create: `tests/entity_system_tests.cpp`
- Modify: `src/game/GameState.hpp`
- Modify: `src/api/ApiDispatcher.cpp`

**Interfaces:**

```cpp
class EntityRegistry {
public:
    int add(std::unique_ptr<Entity> entity);
    Entity* find(int id) noexcept;
    const Entity* find(int id) const noexcept;
    std::vector<int> ids() const;
};
```

- [ ] **Step 1: RED tests** prove stable IDs, missing-ID handling, local-player ID visibility, and that entity API calls no longer alias arbitrary IDs to player 0.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement registry and local player registration**.
- [ ] **Step 4: Generate `BlockHit` from actual client raycast interaction and retain API polling/clear semantics**.
- [ ] **Step 5: GREEN + commit** `feat: add entity registry and real hit events`.

### Task 11: Fixed simulation/game loop

**Files:**
- Create: `src/game/GameLoop.hpp`
- Create: `src/game/GameLoop.cpp`
- Create: `tests/game_loop_tests.cpp`
- Modify: `src/main.cpp`
- Modify: `src/client/ClientApp.cpp`

**Interfaces:**

```cpp
class GameLoop {
public:
    static constexpr double provisional_tick_seconds = 1.0 / 20.0;
    void tick(GameState&);
};
```

- [ ] **Step 1: RED tests** for deterministic tick count, player-before/after world update order as currently documented, pause suppression, and scheduled block tick delivery.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement accumulator-based fixed-step loop separated from rendering frequency**.
- [ ] **Step 4: Record tick order in test instrumentation so original-derived traces can replace provisional ordering without API redesign**.
- [ ] **Step 5: GREEN + commit** `feat: introduce deterministic simulation loop`.

### Task 12: Reachable networking boundary

**Files:**
- Create: `src/network/Packet.hpp`
- Create: `src/network/Packet.cpp`
- Create: `src/network/NetworkHandler.hpp`
- Create: `src/network/NetworkHandler.cpp`
- Create: `tests/network_parity_tests.cpp`
- Create: `docs/reverse-engineering/networking.md`
- Modify: `CMakeLists.txt`

**Interfaces:**

```cpp
struct PacketFrame { std::uint8_t id; std::vector<std::uint8_t> payload; };
class NetworkHandler {
public:
    virtual ~NetworkHandler() = default;
    virtual void handle(const PacketFrame&) = 0;
};
```

- [ ] **Step 1: RED tests** for safe frame bounds, unknown packet preservation/rejection policy, and deterministic encode/decode round-trip for confirmed packet forms.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement only packet/frame facts supported by RE evidence; no speculative multiplayer UI or server protocol**.
- [ ] **Step 4: Document whether each discovered network path is reachable, dormant, or unknown in Pi 0.1.1**.
- [ ] **Step 5: GREEN + commit** `feat: add evidence-bounded network layer`.

### Task 13: MCPI transcript parity

**Files:**
- Create: `tests/parity/reference/api-transcripts.ref`
- Create: `tests/api_transcript_parity_tests.cpp`
- Modify: `src/api/Command.cpp`
- Modify: `src/api/ApiDispatcher.cpp`
- Modify: `src/api/ApiServer.cpp`

**Interfaces:**
- Consumes: `ReferenceSuite`.
- Produces exact response/no-response classification for each transcript case.

- [ ] **Step 1: RED transcript cases** cover malformed args, float formatting, floor/negative positions, spawn offsets, finite clamps, chat with commas, camera, events, entity IDs, and unknown commands.
- [ ] **Step 2: Run RED** and inspect each mismatch rather than weakening expected results.
- [ ] **Step 3: Refactor dispatcher helpers into explicit `CommandResult { Response, NoResponse, Fail }` semantics**.
- [ ] **Step 4: Implement each observed transcript rule until all committed cases match**.
- [ ] **Step 5: GREEN + commit** `fix: match MCPI command transcripts`.

### Task 14: Java API completion and JAR packaging

**Files:**
- Modify/Create under: `clients/java/src/pi/`
- Create: `tests/java/OriginalApiSurfaceSmoke.java`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `mcpi-java.jar` containing the independent `pi` compatibility client.

- [ ] **Step 1: RED compile test** imports every original-known public class/constant/method from the bundled Pi API specification and source reference.
- [ ] **Step 2: Run RED** for missing surface.
- [ ] **Step 3: Add missing independent wrappers and signatures without copying long proprietary implementation text**.
- [ ] **Step 4: Add `jar --create --file ...` CMake target and integration smoke against `mcpi-recompiled`**.
- [ ] **Step 5: GREEN + commit** `feat: complete Java Pi API compatibility jar`.

### Task 15: Python API/example compatibility

**Files:**
- Create: `tests/python_api_compatibility.py`
- Create: `tests/python_examples_smoke.py`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Uses installed `mcpi==1.2.1` and protocol server; does not vendor the external package.

- [ ] **Step 1: RED tests** exercise block constants, vectors, player/world calls, camera, events, chat, entity operations and multiple sequential commands.
- [ ] **Step 2: Run RED** against current dispatcher/entity behavior.
- [ ] **Step 3: Fix server behavior rather than monkey-patching the Python package**.
- [ ] **Step 4: Add smoke equivalents for bundled original example command patterns that can run headless**.
- [ ] **Step 5: GREEN + commit** `test: complete Python API compatibility coverage`.

### Task 16: Camera model integrated with rendering

**Files:**
- Create: `src/client/Camera.hpp`
- Create: `src/client/Camera.cpp`
- Create: `tests/camera_tests.cpp`
- Modify: `src/game/GameApi.hpp`
- Modify: `src/game/GameState.hpp`
- Modify: `src/client/ClientApp.cpp`

**Interfaces:**

```cpp
struct CameraState { CameraMode mode; Vec3 position; int target_entity_id; double yaw; double pitch; };
class CameraController { public: CameraState resolve(const GameState&) const; };
```

- [ ] **Step 1: RED tests** for normal, third-person offset, fixed `setPos`, follow target, invalid target fallback.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Make GameState camera target/position explicit and let renderer consume resolved camera state**.
- [ ] **Step 4: Add reference transforms/screenshots metadata without committing original image assets**.
- [ ] **Step 5: GREEN + commit** `feat: integrate MCPI camera modes with client`.

### Task 17: Asset source and original-Pi asset locator

**Files:**
- Create: `src/assets/AssetSource.hpp`
- Create: `src/assets/OriginalPiAssetSource.hpp`
- Create: `src/assets/OriginalPiAssetSource.cpp`
- Create: `src/assets/FallbackAssetSource.hpp`
- Create: `src/assets/FallbackAssetSource.cpp`
- Create: `tests/asset_source_tests.cpp`
- Modify: `src/main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**

```cpp
class AssetSource {
public:
    virtual ~AssetSource() = default;
    virtual std::optional<std::vector<std::uint8_t>> read(std::string_view path) const = 0;
};
```

- [ ] **Step 1: RED tests** for explicit `--assets` path, traversal rejection, missing asset fallback, and successful reads from a temporary fake install tree.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement original-install locator with no network download behavior**.
- [ ] **Step 4: Add project-owned tiny fallback texture/font data or procedural generation; do not copy Mojang assets**.
- [ ] **Step 5: GREEN + commit** `feat: load user-provided original Pi assets safely`.

### Task 18: Real voxel chunk renderer

**Files:**
- Create: `src/client/ChunkMesh.hpp`
- Create: `src/client/ChunkMesh.cpp`
- Create: `src/client/LevelRenderer.hpp`
- Create: `src/client/LevelRenderer.cpp`
- Create: `tests/chunk_mesh_tests.cpp`
- Modify: `src/client/ClientApp.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**

```cpp
struct MeshVertex { float x,y,z,u,v,light; };
struct ChunkMesh { std::vector<MeshVertex> opaque; std::vector<MeshVertex> translucent; };
class ChunkMeshBuilder { public: ChunkMesh build(const World&, int chunk_x, int chunk_z) const; };
```

- [ ] **Step 1: RED geometry tests** for one isolated cube = 6 visible faces, adjacent cubes hide shared face, opaque/translucent separation, metadata-selected texture face, light value propagation.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement mesh builder independent of SDL rendering**.
- [ ] **Step 4: Replace height-surface painter in `ClientApp` with depth-tested chunk geometry, texture coordinates, sky/fog, selection outline, and camera matrices**.
- [ ] **Step 5: Add deterministic reconstruction screenshots as CI artifacts, not golden proprietary originals**.
- [ ] **Step 6: GREEN + commit** `feat: replace Phase 1 painter with voxel renderer`.

### Task 19: Audio and original UI/HUD

**Files:**
- Create: `src/client/SoundEngine.hpp`
- Create: `src/client/SoundEngine.cpp`
- Create: `src/client/HudRenderer.hpp`
- Create: `src/client/HudRenderer.cpp`
- Create: `src/client/Screen.hpp`
- Create: `src/client/Screen.cpp`
- Create: `tests/ui_audio_tests.cpp`
- Modify: `src/client/ClientApp.cpp`

**Interfaces:**
- `SoundEngine::play(std::string_view event, Vec3 origin)` resolves through `AssetSource`.
- `HudRenderer` renders hotbar, crosshair, chat and current screen in 848×480 reference coordinates.

- [ ] **Step 1: RED tests** for screen transitions, 848×480 layout anchors, selected hotbar slot rectangle, chat lifetime/order, missing-sound nonfatal behavior, and positional attenuation math.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Implement SDL3 audio stream/mixer wrapper and event lookup with silence fallback**.
- [ ] **Step 4: Implement original-reachable title/pause/HUD states based on documented reference layout; scaling is letterboxed/proportional from 848×480**.
- [ ] **Step 5: GREEN + commit** `feat: reconstruct Pi HUD UI and audio layer`.

### Task 20: Platform matrix and portability fixes

**Files:**
- Modify: `.github/workflows/build.yml`
- Create: `docs/platforms.md`
- Create: `cmake/toolchains/linux-arm64.cmake`
- Create: `cmake/toolchains/linux-arm32.cmake`
- Create: `tests/platform_contract_tests.py`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces reproducible configure/build recipes for Linux x86-64, Windows x86-64, macOS x86-64/arm64, Linux ARM64 and historical ARM32 cross build.

- [ ] **Step 1: RED workflow-contract test** requiring macOS job plus checked-in ARM toolchain files and documentation commands.
- [ ] **Step 2: Run RED**.
- [ ] **Step 3: Add macOS hosted build/test matrix with client enabled where SDL dependencies succeed**.
- [ ] **Step 4: Add ARM64/ARM32 cross-build jobs or configure/build smoke jobs using maintained packages/toolchains available on Ubuntu; runtime-only claims remain separate**.
- [ ] **Step 5: Fix portability warnings/errors in project code without suppressing diagnostics**.
- [ ] **Step 6: GREEN + commit** `ci: expand original parity platform matrix`.

### Task 21: Final cross-subsystem parity acceptance gate

**Files:**
- Create: `tests/phase2_parity_acceptance_tests.cpp`
- Create: `tests/parity_report_tests.py`
- Modify: `docs/parity-status.md`
- Modify: `README.md`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes every subsystem public interface and committed safe reference suite.
- Produces final `phase2_parity_acceptance` CTest and parity-status contract.

- [ ] **Step 1: RED acceptance test** creates/loads a world, validates chunk/light fingerprint, runs simulation movement, uses inventory/entity API, executes transcript cases, resolves camera, builds chunk mesh, saves/reloads, and checks deterministic state digest.
- [ ] **Step 2: RED parity-report test** rejects any `matched` status row without a non-empty evidence path and acceptance-test name.
- [ ] **Step 3: Run RED** and fix subsystem regressions rather than bypassing them.
- [ ] **Step 4: Update `docs/parity-status.md` row-by-row from actual test/evidence outcomes; leave unresolved rows `partial` or `unknown` rather than forcing green claims**.
- [ ] **Step 5: Update README Phase 2 section with exact matched/nonmatched scope and asset instructions**.
- [ ] **Step 6: Run full Linux and Windows suites plus every available expanded-platform job**.
- [ ] **Step 7: Commit** `test: add Phase 2 original parity acceptance gate`.

## Self-review

- Spec coverage: Tasks 4–20 map to all 20 acceptance areas; Tasks 1–3 provide the oracle and RE infrastructure; Task 21 prevents unsupported completion claims.
- Placeholder scan: The plan intentionally uses `partial`/`unknown` as parity statuses, not implementation placeholders. No task instructs the implementer to invent unknown original behavior.
- Type consistency: `ReferenceCase`, `ReferenceSuite`, `LightEngine`, `BlockBehaviorRegistry`, `Inventory`, `Entity`, `Player`, `GameLoop`, `AssetSource`, camera, mesh and storage interfaces are defined once and reused consistently.
- Legal boundary: Original executable/assets remain local-only reference inputs; committed data is limited to independently derived behavioral vectors and project-owned fallbacks.