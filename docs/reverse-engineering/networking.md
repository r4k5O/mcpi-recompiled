# Minecraft: Pi Edition 0.1.1 networking evidence

This document tracks networking facts that are supported by the supplied original Pi 0.1.1 ARM ELF without assuming that inherited Minecraft: Pocket Edition networking paths were reachable in the shipped Pi game.

## Confirmed structural anchors

The original binary preserves RTTI/typeinfo/vtable evidence for:

- `ClientSideNetworkHandler`
- `ServerSideNetworkHandler`

The machine-readable anchor addresses are recorded in `anchor-index.tsv`.

The binary also preserves type-name evidence for these packet classes:

- `RequestChunkPacket`
- `ChunkDataPacket`
- `MovePlayerPacket`
- `PlaceBlockPacket`
- `UpdateBlockPacket`

Class presence is not runtime-reachability evidence.

## Reachability table

| Path/type | Presence | Pi 0.1.1 reachability | Wire ID | Payload layout | Current reconstruction policy |
|---|---|---|---|---|---|
| `ClientSideNetworkHandler` | confirmed RTTI/vtable | unknown | n/a | n/a | structural boundary only |
| `ServerSideNetworkHandler` | confirmed RTTI/vtable | unknown | n/a | n/a | structural boundary only |
| `RequestChunkPacket` | confirmed type name | unknown | unknown | unknown | preserve opaque frames only |
| `ChunkDataPacket` | confirmed type name | unknown | unknown | unknown | preserve opaque frames only |
| `MovePlayerPacket` | confirmed type name | unknown | unknown | unknown | preserve opaque frames only |
| `PlaceBlockPacket` | confirmed type name | unknown | unknown | unknown | preserve opaque frames only |
| `UpdateBlockPacket` | confirmed type name | unknown | unknown | unknown | preserve opaque frames only |

No packet ID or payload field is assigned until one is independently traced or measured. This deliberately avoids importing Pocket Edition protocol tables merely because Pi Edition has MCPE ancestry.

## Reconstruction boundary

`src/network/Packet.*` defines an internal evidence-safe `PacketFrame` consisting of one opaque identifier byte plus an opaque payload. The encode/decode helpers are **not claimed to be the original Pi transport framing**. They are a deterministic reconstruction boundary used to:

1. retain unknown packet bytes without silently interpreting them;
2. enforce bounded input before future packet parsers are introduced;
3. give parity tests a stable place to attach evidence as original layouts are recovered.

Unknown packet identifiers are preserved instead of rejected as unsupported semantics. Oversized or empty frames are rejected at the reconstruction boundary before payload interpretation.

## Separate MCPI API

The TCP MCPI API on port 4711 is a separate subsystem. Its ASCII line protocol must not be conflated with the client/server packet classes listed here.

## Evidence needed before promotion

A packet/path can move from `unknown` to `reachable` or `dormant` only with concrete evidence such as:

- a call graph from normal Pi startup/world play into the relevant handler;
- a branch/setting that demonstrably enables or disables that path;
- a captured original execution trace showing the path;
- packet constructor/serializer/parser tracing sufficient to establish an ID/layout.

Until then, the correct parity claim is structural presence with unknown runtime reachability.
