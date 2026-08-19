# MCPI API coordinate translation — original 0.1.1

This note records the coordinate translation used by the original Minecraft: Pi Edition 0.1.1 programming API.

## Result

**Confirmed:** MCPI API world positions are translated relative to the world's spawn coordinates before they reach internal Level coordinates.

Conceptually:

```text
incoming API position -> internal world position
internal = api + spawn

internal world position -> outgoing API position
api = internal - spawn
```

The implementation uses an `OffsetPosTranslator`-like object embedded in the API handler.

## Type identification

The binary contains RTTI/type strings:

```text
14IPosTranslator
19OffsetPosTranslator
```

The candidate translator vtable is around virtual address `0x00103838`.

The API-handler object stores the translator beginning at handler offset `+0x1c`. The translator holds three float offsets at its own offsets:

```text
+0x04
+0x08
+0x0c
```

## Translator operations

### Integer incoming transform

Function `0x00027c98` takes three integer coordinates by pointer and subtracts the translator's three float offsets after converting those offsets to integers.

The API handler calls this function before internal world operations such as:

- `world.setBlock`
- `world.getBlock`
- `world.getBlockWithData`
- both endpoints of `world.setBlocks`

### Integer outgoing transform

Function `0x00027c14` performs the opposite operation: it adds the translator offsets to integer coordinates.

Float variants exist around:

- `0x00027be0` — add offsets
- `0x00027c64` — subtract offsets

These are used by player/entity exact-position commands.

## Why the offset is the world spawn

The API-handler initialization path at approximately `0x0006a704` obtains a three-integer vector from the Level through `0x000a6010`, negates all three values, converts them to floats, and stores them in the translator at handler offsets `+0x20`, `+0x24`, `+0x28` (translator `+0x04/+0x08/+0x0c`).

Function `0x000a6010` reads three adjacent LevelData fields through getters at:

```text
0x000ba958 -> LevelData +0x5c
0x000ba960 -> LevelData +0x60
0x000ba968 -> LevelData +0x64
```

The LevelData/NBT loading code around `0x000bac60` binds those exact fields to the literal keys:

```text
LevelData +0x5c <- "SpawnX"
LevelData +0x60 <- "SpawnY"
LevelData +0x64 <- "SpawnZ"
```

Therefore the translator stores:

```text
offset = (-SpawnX, -SpawnY, -SpawnZ)
```

and the previously observed operations yield:

```text
incoming:
internal = api - offset
         = api + spawn

outgoing:
api = internal + offset
    = internal - spawn
```

This is direct evidence, not a guess based on modern Minecraft behavior.

## Internal world bounds observed in `world.setBlocks`

The original `world.setBlocks` handler parses both endpoints, translates them through the spawn-relative translator, orders min/max endpoints, and then clamps the resulting internal coordinates before iterating.

Observed upper clamps include:

```text
X/Z-like axes: 255
Y-like axis:   127
```

and negative values are clamped to zero in that bulk-operation path.

This is consistent with the finite Pi Edition world and the separately confirmed LevelChunk internal Y range `0..127`.

The exact treatment of out-of-range coordinates for every individual command should still be mapped command-by-command; do not generalize the `setBlocks` clamp behavior to all API calls without evidence.

## Relationship to Java `Vec.MIN_Y/MAX_Y`

The distributed Java source defines:

```text
MIN_Y = -128
MAX_Y = 127
```

Those are public API-coordinate constants. They are **not** the same thing as the internal LevelChunk Y index range. Because API coordinates are spawn-relative, an external API Y value is translated by `SpawnY` before internal Level access.

## Reconstruction consequence

`mcpi-recompiled` currently passes API coordinates directly into `GameState`. To reproduce the original API accurately, a compatibility layer should eventually own an explicit world-origin/spawn translator rather than baking translation into `World` or `LevelChunk`.

Recommended seam:

```text
MCPI command
    ↓
ApiDispatcher
    ↓
ApiCoordinateTranslator  (spawn-relative compatibility behavior)
    ↓
Game/Level internal coordinates
```

That keeps the reconstructed engine's internal coordinates independent of the historical scripting API convention and allows player/entity/world commands to share the same verified transform.
