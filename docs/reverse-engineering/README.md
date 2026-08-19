# Reverse engineering notes

This directory contains evidence and reconstruction notes for the original Minecraft: Pi Edition 0.1.1 executable.

## Boundary

The original `minecraft-pi` executable and Mojang-owned game assets are **reference inputs only** and are not committed to this repository.

The repository may contain:

- hashes and metadata identifying a reference build;
- addresses, function boundaries, class/type names, constants, imports, and behavioral observations;
- independently written pseudocode summaries;
- compatibility tests derived from observed behavior;
- independently written C++ implementing the reconstructed behavior.

The repository should not contain copied proprietary decompiler output, a modified original executable, or redistributed original game assets.

## Method

For each subsystem:

1. identify stable anchors (RTTI strings, protocol strings, imports, vtables, unwind entries);
2. map candidate functions and object layouts;
3. separate **confirmed observations** from **inferences**;
4. write a behavioral test when possible;
5. implement the smallest independently written equivalent;
6. compare against the original reference behavior.

Current tooling is intentionally simple and reproducible: `file`, `readelf`, `strings`, `javap`, and LLVM `objdump` are sufficient for the initial ARM mapping pass. A GUI decompiler can be added later, but its output remains a research aid rather than source code for direct copying.
