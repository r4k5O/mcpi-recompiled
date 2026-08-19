# LevelChunk storage layout — MCPI 0.1.1

This note records the first evidence-backed reconstruction of the original `LevelChunk` storage model. Addresses refer to the supplied original Minecraft: Pi Edition 0.1.1 ARM executable identified in `mcpi-0.1.1-initial-map.md`.

## Confidence legend

- **Confirmed** — directly visible from control flow, field access, allocation size, or byte/nibble indexing.
- **Strong inference** — multiple independent observations fit one interpretation, but stripped symbols do not directly name the member.
- **Open** — hypothesis only; do not encode as compatibility behavior yet.

## Horizontal and vertical dimensions

**Confirmed:** block IDs are indexed as:

```text
index = (x << 11) | (z << 7) | y
```

with local `x,z` in `0..15` and internal `y` in `0..127`.

Equivalent arithmetic:

```text
index = x * 2048 + z * 128 + y
```

Therefore:

```text
16 * 16 * 128 = 32768 block positions
```

Y is the fastest-moving coordinate.

## Candidate object layout

| Offset | Size / representation | Interpretation | Confidence |
|---|---:|---|---|
| `+0x00` | pointer | vptr, candidate vtable around `0x00110268` | Confirmed |
| `+0x0c` | pointer | owning `Level*`-like object | Strong inference |
| `+0x10` | 12-byte layer object | block metadata/data nibbles | Strong inference |
| `+0x1c` | 12-byte layer object | sky-light nibbles | Strong inference |
| `+0x28` | 12-byte layer object | block-light nibbles | Strong inference |
| `+0x34` | 256 bytes | 16×16 height map | Strong inference, heavily supported |
| `+0x134` | 256 bytes | per-column bit/state data; exact semantics unresolved | Open |
| `+0x234` | integer | cached vertical/height-related value | Open |
| `+0x238` | int | chunk X | Confirmed |
| `+0x23c` | int | chunk Z | Confirmed |
| `+0x240` | int | chunk world X origin (`chunkX * 16`) | Confirmed |
| `+0x244` | int | chunk world Z origin (`chunkZ * 16`) | Confirmed |
| `+0x248..+0x24c` | bytes | flags / dirty-state fields | Confirmed as storage; semantics open |
| `+0x250` | integer/pointer-sized field | unresolved | Open |
| `+0x254` | pointer | 32,768-byte block-ID array | Confirmed |

The object also contains additional structures beginning around `+0x258`/`+0x258 + ...`; those are intentionally not named yet.

## Block IDs

### Getter

Function candidate around `0x000af9d0` computes the block index and performs a byte load through the pointer at object offset `+0x254`.

This makes the primary block-ID buffer **32,768 bytes**.

### Writer

Function `0x000affdc` computes the same linear index and stores one byte into the `+0x254` buffer.

## Three nibble layers

### Allocation helper

Function `0x000b1368` behaves like a compact nibble-array constructor:

1. receives a logical element count in argument 2;
2. arithmetic-shifts that count right by one;
3. stores the half-size in the layer object;
4. allocates that many bytes;
5. zero-fills the allocation;
6. records the same byte count as the end/capacity-like field.

### LevelChunk constructor path

The construction path beginning around `0x000b0a34` sets a logical size of `32768` and calls `0x000b1368` three times for members at:

```text
+0x10
+0x1c
+0x28
```

Therefore each layer owns:

```text
32768 / 2 = 16384 bytes
```

and stores one 4-bit nibble per block position.

### Nibble getter/setter

Functions `0x000b13bc` and `0x000b13dc` use the same block index as block IDs, then:

```text
byte_index = index >> 1
```

- even block index → low nibble
- odd block index → high nibble

This is direct evidence that these members are three independent 4-bit-per-block data layers.

## Layer semantics

### `+0x10`: block metadata/data

**Strong inference.** A dedicated getter at `0x000aff30` and setter at `0x000aff88` access only the `+0x10` nibble layer and do not require a light-layer selector. That access shape matches block metadata/data much better than lighting.

This interpretation is also consistent with chunk serialization at `0x000afc04`, which copies block IDs followed by all three nibble layers as independent packed regions.

### `+0x1c` and `+0x28`: sky light and block light

**Strong inference.** Functions around `0x000afee4` / `0x000aff38` choose between `+0x1c` and `+0x28` by comparing a caller-supplied selector with global objects at `0x0017be90` and `0x0017be94`.

Startup/static initialization at `0x000116a4` writes:

```text
0x0017be90 = 15
0x0017be94 = 0
```

Those defaults strongly match the two Minecraft light-layer defaults:

- sky light: full brightness (`15`)
- block light: no emitted light (`0`)

Accordingly the current best mapping is:

```text
+0x1c → sky light
+0x28 → block light
```

This mapping should remain marked as **strong inference** until an independently identified lighting calculation or call site proves the selector names.

## Chunk serialization order

Function `0x000afc04` copies selected chunk ranges in this order:

1. block IDs through `+0x254` (1 byte/block)
2. nibble layer at `+0x10`
3. nibble layer at `+0x28`
4. nibble layer at `+0x1c`

The order is confirmed; the semantic labels of the three nibble regions carry the confidence levels above.

## Height map

Function around `0x000af9bc` reads one byte at:

```text
object + 0x34 + (x | (z << 4))
```

This addresses exactly 256 bytes for a 16×16 column grid.

A routine around `0x000afff4` scans the block-ID buffer vertically and updates this byte per column when column height changes. Together these observations make the `+0x34` member a very strong height-map identification.

## `+0x134` column data

`0x000aff90` zeroes a second 256-byte region at `+0x134`. Other code sets individual bits based on a shifted Y-derived value. The exact meaning is not yet assigned; it may track per-column update/dirty information, but that remains **open**.

## Reconstruction implication

Our current `mcpi-recompiled` chunk implementation is intentionally sparse. The original engine evidence now supports a future compatibility-oriented representation shaped approximately as:

```text
LevelChunk
├── blockIds[32768]          // uint8
├── metadata[16384]          // packed nibbles
├── skyLight[16384]          // packed nibbles (strong inference)
├── blockLight[16384]        // packed nibbles (strong inference)
├── heightMap[256]           // uint8 (strong inference)
└── additional state/flags   // still being mapped
```

Do not implement the light-layer names solely from this document without keeping the confidence distinction visible in tests/notes. The next useful evidence target is the light propagation/calculation path, followed by the external-API-Y to internal-Y translation.
