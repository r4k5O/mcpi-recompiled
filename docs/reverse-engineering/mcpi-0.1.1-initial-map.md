# Minecraft: Pi Edition 0.1.1 — initial binary map

Reference: the original `minecraft-pi` executable from the Minecraft: Pi Edition **0.1.1 alpha** distribution supplied for analysis.

## Reference identity

| Property | Observed value |
|---|---|
| file size | `1,206,760` bytes |
| SHA-256 | `45280c16930d3c787412a8c6239df77d8b34ad6d2698e01d73f94d914080f085` |
| format | ELF32, little-endian, ARM, EABI5 |
| linkage | dynamically linked |
| interpreter | `/lib/ld-linux-armhf.so.3` |
| entry point | `0x12360` |
| `.text` | `0x0000de60`, size `0x0f4260` |
| `.rodata` | `0x001020c8`, size `0x014594` |
| compiler evidence | GCC (Debian/Raspberry Pi) 4.6.3 |
| ARM unwind entries | 4,284 |

The large `.ARM.exidx` table is especially useful because it preserves thousands of function-start candidates even though the executable is stripped.

## Runtime dependencies

The executable declares these shared-library dependencies:

- `libGLESv2.so`
- `libEGL.so`
- `libbcm_host.so`
- `libpng12.so.0`
- `libSDL-1.2.so.0`
- `libstdc++.so.6`
- `libm.so.6`
- `libgcc_s.so.1`
- `libc.so.6`
- `libpthread.so.0`
- `libX11.so.6`

This gives us a concrete platform-replacement map: VideoCore/bcm_host + EGL/GLES + SDL 1.2/X11 are platform concerns; game logic should remain above that boundary in the reconstruction.

## Startup / main

### Confirmed

The ELF entry point at `0x12360` passes `0x0000dfd8` as the program's `main`-style function to `__libc_start_main`.

At `0x0000dfd8`, the startup path includes the following observable sequence:

1. `bcm_host_init()`
2. `SDL_Init(0x20)` (`0x20` is the video-init flag in the SDL 1.2 API)
3. `SDL_SetVideoMode(848, 480, 32, 16)`
4. window caption string: `Minecraft - Pi edition`
5. allocate `3280` bytes for the main app object
6. call candidate app constructor at `0x000147b4`
7. read `HOME` and append `/.minecraft/`
8. enter an event/update loop involving functions at `0x00012750` and `0x000128a8`
9. call a virtual method through the app object's vtable inside the loop

The binary contains RTTI text `12NinecraftApp`; the constructor at `0x147b4` writes vtable address `0x001023b8` into the object. The spelling is recorded exactly as present in the binary and should not be silently corrected until inheritance/type relationships are mapped.

### Inference

`0x147b4` is very likely the constructor of the top-level application object used by `main`. This is supported by the allocation size, immediate constructor call, and vtable write, but the final class/function name remains an inference until the surrounding RTTI/vtable relationship is fully mapped.

## Version ancestry evidence

The binary contains the literal version string `v0.6.1` near the top-level application RTTI/string region. This is direct binary evidence that should be considered when comparing Pi Edition behavior against the Pocket Edition 0.6.1 code/behavior lineage. It does **not** by itself prove that every subsystem is identical to MCPE 0.6.1.

## High-value RTTI / subsystem anchors

Confirmed type-name strings include, among many others:

- `Minecraft`
- `Level`
- `LevelChunk`
- `LevelSource`
- `ChunkSource`
- `ChunkCache`
- `EmptyLevelChunk`
- `Dimension`
- `RandomLevelSource`
- `LevelStorage`
- `ChunkStorage`
- `ExternalFileLevelStorage`
- `LevelStorageSource`
- `CreatorLevel`
- `ServerLevel`
- `MultiPlayerLevel`
- `LocalPlayer`
- `RemotePlayer`
- `ServerPlayer`
- `Entity`
- `Inventory`
- `LevelRenderer`
- `EntityRenderer`
- `PlayerRenderer`
- `ClientSideNetworkHandler`
- `ServerSideNetworkHandler`
- packet types including `RequestChunkPacket`, `ChunkDataPacket`, `MovePlayerPacket`, `PlaceBlockPacket`, and `UpdateBlockPacket`

These anchors allow class/vtable mapping without guessing subsystem names.

## LevelChunk: first decompiled layout findings

This is the first game-engine class for which we have useful internal layout evidence.

### Vtable

The `LevelChunk` RTTI name is at virtual address `0x001102f0`; its type-info object is associated with the nearby RTTI/vtable region, and the candidate LevelChunk vtable's callable entries begin at approximately `0x00110268`.

### Candidate constructor

Function `0x000b0b08` is a strong `LevelChunk` constructor candidate. Observed field writes are consistent with arguments resembling `(level, chunkX, chunkZ)`:

| object offset | observation |
|---|---|
| `+0x00` | LevelChunk candidate vptr (`0x00110268`) |
| `+0x0c` | pointer supplied by argument 1; likely owning `Level*` |
| `+0x238` | chunk X coordinate |
| `+0x23c` | chunk Z coordinate |
| `+0x240` | chunk X multiplied by 16 (world-space X origin) |
| `+0x244` | chunk Z multiplied by 16 (world-space Z origin) |
| `+0x249..+0x24c` | byte-sized state/dirty flags (exact meanings not yet assigned) |
| `+0x254` | pointer used as the main block-ID storage buffer |

A second construction path stores an externally supplied pointer into `+0x254`, which suggests LevelChunk can be initialized from an existing block buffer as well as created empty.

### Block storage dimensions and indexing

Function `0x000af9d0` reads one byte from the buffer at `+0x254` with the effective index:

`(x << 11) | (z << 7) | y`

For local coordinates `x,z = 0..15` and internal `y = 0..127`, this spans exactly `0..32767`:

- X stride: `2048`
- Z stride: `128`
- Y stride: `1`
- total block-ID bytes per chunk: `16 * 16 * 128 = 32768`

This is strong evidence that the original engine's `LevelChunk` uses a **16 × 128 × 16** byte block-ID array with Y as the fastest-moving coordinate.

### Height map

Function `0x000af9bc` reads a byte at object offset `0x34 + (x | (z << 4))`. This is consistent with a `16 × 16` one-byte height map stored directly in the LevelChunk object.

Another routine around `0x000af8ec` scans the block buffer from Y=126 downward and updates that 16×16 height data, providing additional support for the interpretation.

### Why this matters for `mcpi-recompiled`

Our current chunk storage is intentionally sparse and has no fixed internal height. It should remain provisional until this LevelChunk mapping is implemented behind tests. The next reconstruction PR can now be evidence-driven rather than choosing a layout from memory.

## API evidence

The executable itself contains the original API command strings, including:

- `world.setBlock`
- `world.getBlock`
- `world.getBlockWithData`
- `world.setBlocks`
- `world.getHeight`
- `world.checkpoint.save`
- `world.checkpoint.restore`
- `world.setting`
- `player.setTile`
- `player.getTile`
- `player.setPos`
- `player.getPos`
- `entity.setTile`
- `entity.getTile`
- `entity.setPos`
- `entity.getPos`
- `events.block.hits`
- camera mode/position commands

The distributed `MCPI-PROTOCOL 0.1` specification independently confirms TCP port `4711`, ASCII LF-terminated lines, block IDs `0..108`, block data `0..15`, and the core command set.

## Java API reference

The original release includes actual Java source under `api/java/src-api/pi/`, not merely `McPi.jar`. Therefore Java compatibility work does **not** require decompiling Mojang's Java library.

The public API includes `pi.Minecraft`, `Block`, `Vec`, `VecFloat`, nested Player/Camera/Events/Entities helpers, and utilities. The original `Vec` source/class metadata exposes API coordinate constants:

- `MIN_Y = -128`
- `MAX_Y = 127`

These are API coordinate bounds and should not be confused with the LevelChunk's confirmed internal `0..127` Y indexing; the transformation between external API coordinates and internal world Y remains a reverse-engineering target.

## Confidence legend

- **Confirmed**: direct value/string/control-flow/field-access evidence from the supplied original binary or source/spec distributed with it.
- **Strong inference**: multiple independent observations agree, but a symbol name or semantic role is not directly preserved.
- **Open**: a working hypothesis only; do not encode as compatibility behavior yet.

## Next targets

1. map the `LevelChunk` constructor family and ownership/allocation of the `32768`-byte block buffer;
2. identify get/set block-data (metadata/nibble) storage alongside block IDs;
3. map the external API Y coordinate translation into internal Level/LevelChunk Y;
4. identify `Level` → `ChunkSource` → `LevelChunk` lookup functions;
5. map `RandomLevelSource` enough to reproduce original terrain generation deterministically;
6. map the API command-dispatch handler and compare each command's exact edge behavior;
7. only after evidence is sufficient, replace the provisional sparse chunk backend with a compatibility-oriented fixed LevelChunk representation.
