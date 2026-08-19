# mcpi-recompiled

A source-reconstruction and native-port project for **Minecraft: Pi Edition 0.1.1 alpha**, focused first on reproducing the original game and its programming API on modern systems.

> [!IMPORTANT]
> This project is unofficial and is not affiliated with Mojang Studios or Microsoft. It does **not** redistribute the original `minecraft-pi` executable or game assets. Users must obtain any original files they are legally entitled to use themselves.

## Phase 1 goal

Before adding modern features, the goal is compatibility:

- build natively on modern Linux x86-64 and Windows x86-64;
- reconstruct the original game behavior incrementally;
- preserve compatibility with the Minecraft Pi programming API;
- support the original MCPI protocol on TCP port `4711`;
- keep original binaries/assets out of Git history;
- use tests and documented observations to track parity.

## Status

🚧 **Bootstrap / research stage.** The repository structure, build system, API compatibility harness, and reconstruction notes are being established first.

## Reference target

The initial compatibility target is **Minecraft: Pi Edition 0.1.1 alpha**. The original release contains protocol/API reference material which can be used to validate behavior, while original game binaries and assets stay outside this repository.

## Build

```bash
cmake -S . -B build -DMCPI_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Roadmap

1. Bootstrap portable C++ build + CI.
2. Implement a protocol-compatible API test harness.
3. Document original executable/platform assumptions.
4. Reconstruct platform-independent game systems in small, testable pieces.
5. Bring up rendering/input/windowing on modern systems.
6. Reach playable Pi Edition parity.
7. Only then consider optional modernization.

## Repository policy

Do not commit original Mojang executables, textures, sounds, packaged game data, or leaked/proprietary source code. Keep reverse-engineering notes factual and source their observations.

## License

Licensing for newly written project code will be documented before the first public release. Original Minecraft Pi files remain subject to their own terms and are not part of this repository.
