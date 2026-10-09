# CCSDSPack v2.1.0

> [!IMPORTANT]
> These are pre-tag v2.1.0 release notes. Earlier Raspberry Pi 5 and physical Cortex-M7 runs passed on the source commits recorded in `docs/V2_HARDWARE_VALIDATION.md`; the pre-tag audit updates host/MCU API boundaries. The final accepted post-audit `main` commit must pass CI and fresh native/physical target validation before the release tag is created.

## Summary

CCSDSPack v2.1.0 introduces an authoritative **C11 protocol core** while preserving the established **C++17 Packet/Manager API** as the ownership and convenience layer.

The release is additive within the v2 line. Existing C++ applications keep the v2 API, while C applications can now link the protocol core directly without requiring a C++ compiler or runtime.

The migration also removes several avoidable copies and allocations from the existing C++ paths rather than merely adding a parallel C API.

## C11 protocol core

The installed `ccsdspack::c` target provides caller-owned, allocation-free low-level APIs for:

- raw buffer/view types and explicit big/little byte-order helpers;
- CRC16;
- CCSDS primary-header validation, encoding, decoding, and declared packet sizing;
- zero-copy packet inspection and bounded packet-stream walking;
- packet finalization and caller-buffer encoding;
- numeric CUC time;
- PUS-A/PUS-C TC/TM codecs and tailoring validation;
- structured validation and sequence/segmentation state;
- segmentation planning and sequence generation;
- receive-side application-data reassembly.

The core can be configured and built with `CCSDSPACK_BUILD_CPP=OFF` and `CXX=/bin/false`. CI verifies that the C-only test/consumer does not depend on `libstdc++`.

## C++17 compatibility layer

The existing `ccsdspack::CCSDSPack` target remains available.

The public C++ ownership model is unchanged:

- `ccsds::Packet` owns packet state;
- concrete secondary-header objects own their layout/state;
- `ccsds::Manager` owns packet streams and application-data workflows;
- `Result<T>`, vectors, shared pointers, configuration support, and hosted tools remain available.

Protocol mechanics progressively delegate to the C core. In particular:

- raw bounded parsing is pointer-native;
- primary-header, CRC, CUC and PUS wire codecs delegate to C;
- Validator packet/template/PUS coherence uses the C validation engine;
- Manager segmentation, reassembly, and stream walking use C primitives;
- Packet finalization/encoding uses caller-buffer C APIs;
- packet serialization no longer builds a combined secondary-header/application-data temporary.

## Endianness

The C core exposes explicit byte-order load/store helpers for big- and little-endian wire fields. CCSDS/PUS/CUC standards-defined wire fields remain encoded in their required network/big-endian form; the generic byte helpers allow non-CCSDS integrations to select byte order explicitly without host-endian assumptions.

## Performance versus v2.0 main

Matched Release builds were compared back-to-back on the same Ubuntu 24.04 GitHub Actions runner using v2.0 `main` commit `4e198ae4c7f730737d78c1ea2f71ec3ce42ca7eb` and the v2.1 implementation under qualification. The final broader characterization is recorded in `docs/PERFORMANCE.md`.

Representative results:

- C++ owning raw parse allocations: **9/11 -> 3** allocations per operation;
- allocated bytes per parse: approximately **50–83% lower** across the measured range;
- no-CRC C++ parse: **1.6x to 4.8x faster** across 64 B to 32 KiB packets in the matched run;
- CRC-enabled C++ parse: **1.02x to 1.19x faster**, with checksum scanning dominating large packets;
- vector-returning cached serialization: **3 -> 1 allocation**;
- forced re-finalization serialization: **4/6 -> 1 allocation** depending on PEC mode;
- the low-level C packet view remains **0 allocations** and **0 transport-adapter bridge copies**.

Exact methodology, packet sizes, timings and allocation counts are recorded in `docs/PERFORMANCE.md`.
The completed broader matched characterization also shows PUS-A/PUS-C encode/decode speedups
of roughly 1.24x to 2.99x, Manager reassembly at 1.64x with allocations reduced 24 -> 1,
Manager stream loading at 1.10x with materially lower allocation pressure, effectively
unchanged isolated CRC16 throughput, and essentially flat Validator timing with fewer
allocations. Numeric CUC encode/decode is the documented exception and is slower in the
matched C++ wrapper benchmark; focused optimization is tracked in issue #166.

## Cortex-M7 footprint

The same v2.0 MCU compile-probe source was linked against both implementations with Cortex-M7 hard-float flags, `-Os`, function/data sections, no RTTI, and no exceptions.

With section garbage collection rooted at the probe entry point:

- v2.0 main retained text: **36,238 bytes**;
- v2.1 retained text: **40,166 bytes**;
- delta: **+3,928 bytes (+10.8%)**;
- data/bss in the relocatable comparison: **0/0** for both.

The symbol review shows the increase is distributed across the newly retained C packet/PUS/CUC/validation/stream primitives rather than one obvious duplicate implementation. Physical STM32 execution passed on the qualified v2.1 implementation; the matched compile/link comparison remains the cross-version footprint metric.

## Standards scope

The standards scope is unchanged from v2.0.0:

- CCSDS 133.0-B-2 Issue 2, including Editorial Change 2, for the supported Space Packet PDU profile;
- ECSS-E-70-41A for supported PUS-A TC/TM layouts;
- ECSS-E-ST-70-41C for supported PUS-C TC/TM layouts;
- CCSDS 301.0-B-4 for the supported basic numeric CUC subset.

The release does not claim complete PUS services, transfer frames, COP-1, CFDP, UTC/leap-second conversion, or mission time correlation.

## Validation and robustness

The existing v2 regression/conformance suite remains the C++ compatibility gate.

v2.1 additionally runs:

- native C11 core regression tests;
- a true C-only configuration with no C++ compiler;
- installed pure-C package consumer;
- installed C++ package consumer and examples;
- Linux Ubuntu 22.04/24.04/26.04;
- Windows latest;
- Doxygen;
- Clang ASan and UBSan;
- bounded libFuzzer smoke tests;
- native and cross-package generation;
- Cortex-M7 compile/link probe;
- standalone NUCLEO-H755ZI-Q DAS/OpenOCD firmware cross-build from the generated MCU package;
- aarch64 package generation;
- non-gating parse and serialization performance artifacts.

The physical H755 release harness is independent of STM32CubeIDE, STM32 HAL,
the Nucleo BSP and generated vendor IDE projects. DAS supplies startup, vector
table, linker script, board clocking and the ST-LINK VCP UART; OpenOCD owns
program/verify/reset. The target prints build/runtime identity, per-stage
acceptance progress, symbolic failure information and fault markers over
115200 8N1 UART.

## Build and installed targets

Default source build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Pure C11 core:

```bash
CC=gcc CXX=/bin/false cmake -S . -B build-c \
  -DCMAKE_BUILD_TYPE=Release \
  -DCCSDSPACK_BUILD_CPP=OFF \
  -DCCSDSPACK_BUILD_C_TESTS=ON
cmake --build build-c
```

Installed CMake targets:

```cmake
find_package(CCSDSPack 2.1 CONFIG REQUIRED)

# Pure C11 protocol core
target_link_libraries(c_app PRIVATE ccsdspack::c)

# Established C++17 API
target_link_libraries(cpp_app PRIVATE ccsdspack::CCSDSPack)
```

## Remaining release gates

Before publication is complete:

1. merge the reviewed pre-tag audit into `main` and qualify that exact commit in final CI;
2. run native Raspberry Pi 5 and physical Cortex-M7 hardware validation on that exact release commit and record new evidence;
3. create tag `v2.1.0` only after the source and target evidence is accepted;
4. verify tag-produced GitHub Release assets and package hashes;
5. verify GHCR `v2.1.0` and `latest` images.

Previously completed native Raspberry Pi 5 and NUCLEO-H755ZI-Q test runs are recorded in `docs/V2_HARDWARE_VALIDATION.md`; final post-audit hardware acceptance remains outstanding.

## Release control

```text
develop -> main -> final CI -> tag v2.1.0
```

Publication evidence is recorded only after the tag workflow completes.
