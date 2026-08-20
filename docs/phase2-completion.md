# Phase 2 completion

Phase 2 of `mcpi-recompiled` is **implementation-complete**.

This statement closes the implementation plan; it is deliberately **not** a claim that every behavior of Minecraft: Pi Edition 0.1.1 alpha is already proven to be an exact original match. The authoritative evidence status for each subsystem remains [`parity-status.md`](parity-status.md).

## Completion gate

The Phase-2 branch contains all planned reconstruction boundaries and the integrated acceptance gate described by the Phase-2 plan:

- reference-vector and reverse-engineering evidence infrastructure;
- `RandomLevelSource`/chunk-generation reconstruction boundary;
- packed sky/block-light storage and propagation;
- block behavior/update scheduling;
- bounded NBT and Pi/legacy storage routing;
- inventory/hotbar state;
- player/entity/physics state and collision;
- deterministic fixed-step game loop;
- evidence-bounded packet/network-handler layer;
- MCPI response/no-response/`Fail` transcript classification;
- Java 25 compatibility JAR and Python `mcpi` compatibility checks;
- normal, third-person and fixed camera integration;
- local original-asset discovery with project-owned fallbacks;
- visible-face chunk meshing and deterministic software rendering;
- title/game/pause UI and local positional audio;
- Linux, Windows, macOS, ARM32 and ARM64 build contracts;
- cross-subsystem `phase2_parity_acceptance` and `parity_report` tests.

The last pre-closure baseline, commit `af4d6eaf816f0536e4e2a2d8d590df6bffbc12e6`, passed GitHub Actions Build #336 on all six configured jobs. The Linux native job passed all 38 registered tests. Any commits after that baseline must pass the same final CI gate before the branch is considered ready to merge.

## What remains intentionally non-`matched`

Some original behavior cannot be promoted without stronger primary evidence or original-vs-reconstruction reference captures. These are **parity research gaps, not unfinished Phase-2 implementation tasks**. They include, among other items:

- exact original seed expansion, noise algorithms, terrain stages, caves and water behavior;
- exact original lighting/block tables and all tick rules;
- original Pi chunk persistence details;
- exact movement constants and original movement traces;
- broader entity simulation/AI;
- whether inherited multiplayer paths are reachable in the Pi build, plus exact packet wire IDs/layouts;
- pixel-identical renderer/UI behavior and exact sound event/mixing tables;
- exhaustive undocumented MCPI API bugs;
- runtime validation on every cross-compiled target.

Those claims stay `partial`, `confirmed`, or `unknown` until evidence and a named acceptance comparison justify promotion. No Phase-2 completion marker overrides that rule.

## Post-Phase-2 rule

Future work that discovers new original evidence should strengthen reference fixtures and promote individual rows in `parity-status.md` only in the same change that adds a concrete comparison test. Gameplay modernization should remain separate from the original-parity reconstruction line.
