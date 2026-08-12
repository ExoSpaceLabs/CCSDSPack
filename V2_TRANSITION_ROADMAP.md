# CCSDSPack v2.0.0 release roadmap

## Objective

CCSDSPack v2.0.0 provides a standards-oriented C++17 Space Packet library with explicit packet policy, PUS-A/PUS-C secondary-header codecs, numeric CUC time, structured validation, transport-facing buffer APIs, and hosted/bare-metal integration.

The implementation architecture and conformance/robustness evidence are stable. Remaining work is target validation, publication hardening, and final release approval rather than API redesign.

## Implemented product scope

- CCSDS 133.0-B-2 EC2 Space Packet PDU construction, checked serialization, bounded parsing, sequence handling, segmentation, and stream management;
- PUS-A and PUS-C TC/TM concrete secondary-header types with intrinsic revision/direction;
- direction-specific optional PUS tailoring and fixed PUS-C identifier widths;
- numeric basic CUC with validated epoch/P-field/coarse/fine configuration;
- generic Packet-level CRC16/none error-control policy;
- complete Packet templates as the Manager generation/receive contract;
- named fixed-capacity structured validation;
- vector and pointer-plus-size transport interfaces;
- installed CMake package, CLI tools, standalone examples, and package generation;
- C++17 MCU static-library build compatible with `-fno-exceptions -fno-rtti`.

## Current integration evidence

- **132/132 native tests**;
- Ubuntu 22.04, Ubuntu 24.04, Ubuntu latest, and Windows hosted CI;
- Doxygen and CLI integration;
- installed shared-library consumer and examples;
- Ubuntu 22.04 package/cross-build generation;
- Cortex-M compile/link probe;
- independent PUS-C TC acknowledgement vectors for all 16 flag combinations;
- complete 26-code structured validation evidence matrix;
- dedicated Clang ASan and UBSan native-suite CI;
- bounded four-target libFuzzer smoke CI under ASan+UBSan.

## Release-hardening sequence

1. Record fresh native arm64 v2 installed-package/API execution (#87).
2. Record fresh physical STM32H755 v2 PUS/Validator/raw-buffer execution (#87).
3. Correct release publication metadata and version-derived artifact naming, then verify tag-only GitHub Release/GHCR behavior (#87/#88).
4. Add the fresh target/publication results to final compliance evidence and approve release notes (#88).
5. Close completed parent integration issues once #87 acceptance is complete.
6. Promote the approved `develop` release candidate to `main`.
7. Run final `main` CI, tag `v2.0.0`, publish packages, and verify release artifacts (#88).

Feature work branches from `develop` and returns through reviewed pull requests. `develop` is the integration/release-candidate branch; `main` is promoted only after the release-hardening gates are complete.

UML generation remains a manual diagnostic/documentation utility and is not a v2.0.0 release gate.
