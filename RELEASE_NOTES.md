# CCSDSPack v2.1.0

> [!IMPORTANT]
> These notes describe the v2.1.0 release candidate. Hosted CI, cross-builds, package consumers, performance comparison, and Cortex-M7 compile/link evidence are complete. Fresh native arm64 and physical Cortex-M7 execution must still be rerun against the final candidate before the release is promoted to `main` and tagged.

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

Matched Release builds were compared back-to-back on the same Ubuntu 24.04 GitHub Actions runner using v2.0 `main` commit `4e198ae4c7f730737d78c1ea2f71ec3ce42ca7eb` and the v2.1 candidate after PR #154.

Representative results:

- C++ owning raw parse allocations: **9/11 -> 3** allocations per operation;
- allocated bytes per parse: approximately **50–83% lower** across the measured range;
- no-CRC C++ parse: **1.6x to 4.8x faster** across 64 B to 32 KiB packets in the matched run;
- CRC-enabled C++ parse: **1.02x to 1.19x faster**, with checksum scanning dominating large packets;
- vector-returning cached serialization: **3 -> 1 allocation**;
- forced re-finalization serialization: **4/6 -> 1 allocation** depending on PEC mode;
- the low-level C packet view remains **0 allocations** and **0 transport-adapter bridge copies**.

Exact methodology, packet sizes, timings and allocation counts are recorded in `docs/PERFORMANCE.md`.

## Cortex-M7 footprint

The same v2.0 MCU compile-probe source was linked against both implementations with Cortex-M7 hard-float flags, `-Os`, function/data sections, no RTTI, and no exceptions.

With section garbage collection rooted at the probe entry point:

- v2.0 main retained text: **36,238 bytes**;
- v2.1 retained text: **40,166 bytes**;
- delta: **+3,928 bytes (+10.8%)**;
- data/bss in the relocatable comparison: **0/0** for both.

The symbol review shows the increase is distributed across the newly retained C packet/PUS/CUC/validation/stream primitives rather than one obvious duplicate implementation. Physical STM32 ELF size remains a release-candidate validation item.

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
- Linux Ubuntu 22.04/24.04/latest;
- Windows latest;
- Doxygen;
- Clang ASan and UBSan;
- bounded libFuzzer smoke tests;
- native and cross-package generation;
- Cortex-M7 compile/link probe;
- aarch64 package generation;
- non-gating parse and serialization performance artifacts.

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

## Remaining release-candidate gates

Before tagging v2.1.0:

1. merge the final evidence/documentation PR into `develop`;
2. generate fresh v2.1 arm64 and MCU packages from the exact candidate commit;
3. rerun Raspberry Pi 5/native arm64 installed-package acceptance;
4. rerun NUCLEO-H755ZI-Q/Cortex-M7 physical acceptance and record final ELF text/data/bss;
5. promote the accepted `develop` commit to `main`;
6. require final `main` CI to pass;
7. create tag `v2.1.0`;
8. verify tag-produced GitHub Release assets, package hashes, and GHCR images.

## Release control

```text
develop -> physical/native hardware acceptance -> main -> tag v2.1.0
```

Publication evidence is recorded only after the tag workflow completes.
