# Platform build coverage

This document separates **build coverage** from **runtime/original-parity claims**. A successful build proves that the independently written code and its dependencies compile for that target; it does not by itself prove gameplay, graphics, audio, timing, input, networking, or original Minecraft: Pi Edition parity on physical hardware.

## Hosted native CI builds

The normal GitHub Actions matrix builds the full SDL client, Java compatibility JAR, Python compatibility tests, and CTest suite on:

- Linux x86-64 (`ubuntu-latest`)
- Windows x86-64 (`windows-latest`)
- macOS ARM64 (`macos-latest`)
- macOS x86-64 (`macos-15-intel`)

Local native configuration uses the same project switches:

```bash
cmake -S . -B build -DMCPI_BUILD_TESTS=ON -DMCPI_BUILD_CLIENT=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

A green hosted-native CI job is a build/test claim for the reconstructed project on that hosted environment. It is still not evidence that unresolved original behavior is `matched`.

## Full Linux ARM clients under QEMU

CI and release builds now create the **full SDL client** inside target-architecture Ubuntu 22.04 userlands registered through QEMU/binfmt. This avoids mixing x86-64 development libraries with ARM targets and means SDL, X11/OpenGL/EGL and ALSA are compiled and linked for the same architecture as the released executable.

The ARM targets are:

- Linux ARM64: Docker platform `linux/arm64`
- Linux ARM32: Docker platform `linux/arm/v7`

Both configure with:

```bash
cmake -S . -B build-arm \
  -DMCPI_BUILD_TESTS=OFF \
  -DMCPI_BUILD_CLIENT=ON
cmake --build build-arm --config Release --parallel 2
```

The workflow verifies that `build-arm/mcpi-recompiled` exists and is executable and records its ELF architecture with `file` before packaging it.

Release packages are produced alongside the four hosted-native packages:

- `mcpi-recompiled-<tag>-linux-arm64.tar.gz`
- `mcpi-recompiled-<tag>-linux-arm32.tar.gz`

### ARM32 baseline

The current ARM32 release target is **ARMv7** (`linux/arm/v7`). That is intentionally more precise than claiming compatibility with every 32-bit Raspberry Pi. In particular, the original Raspberry Pi 1 uses an ARMv6-class CPU, so the ARMv7 artifact must not be presented as Raspberry Pi 1 compatible without a separate ARMv6/Raspberry Pi OS build and hardware validation.

## Legacy cross-toolchains

The repository still contains `cmake/toolchains/linux-arm64.cmake` and `cmake/toolchains/linux-arm32.cmake` for developer cross-compile experiments and core portability checks. The release workflow no longer uses those toolchains for ARM artifacts because they previously disabled the desktop client.

## Runtime evidence rule

QEMU user-mode builds prove that the full client compiles and links for the target ARM architecture. They do **not** prove that a physical Raspberry Pi has working graphics, audio devices, controller/input behavior, network reachability, or original frame/tick timing. Runtime/platform parity may only be promoted when there is a reproducible execution trace or acceptance test on actual target hardware or a sufficiently representative runtime environment.
