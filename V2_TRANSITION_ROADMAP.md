# CCSDSPack v2.0.0 release roadmap

## Objective

CCSDSPack v2.0.0 provides a standards-oriented C++17 Space Packet library with explicit packet policy, PUS-A/PUS-C secondary-header codecs, numeric CUC time, structured validation, transport-facing buffer APIs, and hosted/bare-metal integration.

The implementation architecture is stable for release hardening. Remaining work is conformance evidence, target validation, CI hardening, and publication rather than API redesign.

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

- 125 native tests;
- Ubuntu 22.04, Ubuntu 24.04, Ubuntu latest, and Windows hosted CI;
- Doxygen and CLI integration;
- installed shared-library consumer and examples;
- Ubuntu 22.04 package/cross-build generation;
- Cortex-M compile/link probe;
- local ASan and UBSan execution.

## Release-hardening sequence

1. Complete and approve the PUS-C acknowledgement vector/traceability matrix (#74).
2. Complete the final structured negative-vector matrix (#76).
3. Add dedicated sanitizer and bounded fuzz smoke CI (#77).
4. Record fresh native arm64 v2 installed-package execution (#87).
5. Record fresh physical STM32H755 v2 PUS/Validator/raw-buffer execution (#87).
6. Reconcile final compliance evidence and release notes (#87/#88).
7. Close completed parent implementation/conformance issues once their child evidence is complete.
8. Promote the approved `develop` release candidate to `main`.
9. Run final `main` CI, tag `v2.0.0`, publish packages, and verify release artifacts (#88).

Branch promotion uses reviewed GitHub pull requests and the repository path `v2.0.0-dev -> develop -> main -> tag v2.0.0`. Individual PRs are the source of truth for promotion status.

UML generation remains a manual diagnostic/documentation utility and is not a v2.0.0 release gate.
