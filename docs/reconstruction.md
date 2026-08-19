# Reconstruction notes

## Reference target

Initial target: **Minecraft: Pi Edition 0.1.1 alpha**.

The local reference archive inspected at project bootstrap contains the original Raspberry Pi release. The game executable is a 32-bit ARM Linux ELF binary, while the release also contains API material describing the MCPI protocol used by external programs.

## Rules for reconstruction

1. Do not commit the original game executable or packaged assets.
2. Do not use leaked or proprietary source code.
3. Record observations separately from assumptions.
4. Prefer behavior tests over guessed implementation details.
5. Keep platform adaptation separate from reconstructed game logic.
6. Do not modernize gameplay until compatibility work is complete.

## Phase 1 compatibility targets

### API

- TCP server on port `4711`.
- Line-oriented MCPI commands.
- Preserve command names, argument ordering, return formats, and event behavior.
- Old Python API examples should eventually run unchanged against the new executable.

### Game

- Startup and shutdown behavior.
- Main menu and world selection/creation.
- World generation and persistence.
- Player movement and interaction.
- Block placement/removal.
- Inventory/hotbar behavior.
- Rendering and input sufficient for original gameplay.

## Suggested reconstruction workflow

For each subsystem:

1. Document observable behavior in the original release.
2. Add a small compatibility test when the behavior can be automated.
3. Identify data structures/functions from binary analysis without importing proprietary code.
4. Write a clean C++ implementation.
5. Compare the port against the original behavior.
6. Mark unknowns explicitly instead of silently guessing.

## API-first strategy

The programming API gives the project a particularly useful black-box test surface. Commands such as world/block operations and player queries can be sent to the original release and later to the reconstructed server, allowing responses and resulting world state to be compared mechanically.

The initial repository code therefore starts with a small MCPI command parser and tests before any renderer or world engine is attempted.
