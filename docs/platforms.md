# Platform build coverage

This document separates **build coverage** from **runtime/original-parity claims**. A successful cross-compile proves that the independently written code is portable enough to compile for that target; it does not prove gameplay, graphics, audio, timing, input, networking, or original Minecraft: Pi Edition parity on physical hardware.

## Native CI builds

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

A green native CI job is a build/test claim for the reconstructed project on that hosted environment. It is still not evidence that unresolved original behavior is `matched`.

## Linux ARM64 cross-build

Install an AArch64 GNU cross compiler on a Debian/Ubuntu host, then configure the project-owned core without the desktop client or host-run tests:

```bash
cmake -S . -B build-arm64 \
  -DMCPI_BUILD_TESTS=OFF \
  -DMCPI_BUILD_CLIENT=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-arm64.cmake
cmake --build build-arm64 --parallel
```

This is a **build smoke** for Linux ARM64. Runtime testing on actual ARM64 hardware remains a separate step.

## Historical Linux ARM32 cross-build

For 32-bit ARM hard-float targets:

```bash
cmake -S . -B build-arm32 \
  -DMCPI_BUILD_TESTS=OFF \
  -DMCPI_BUILD_CLIENT=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-arm32.cmake
cmake --build build-arm32 --parallel
```

This is likewise a **build smoke**, not a Raspberry Pi runtime-parity claim. The original Raspberry Pi hardware/OS environment is not emulated by this job.

## Runtime evidence rule

Runtime/platform parity may only be promoted when there is a reproducible execution trace or acceptance test on that platform. Cross-build jobs deliberately do not claim audio devices, window systems, controller/input behavior, graphics output, network reachability, or frame/tick timing.
