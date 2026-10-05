# CCSDSPack v2 line roadmap

## Current direction

CCSDSPack v2.1 keeps the v2 C++17 Packet/Manager API while moving protocol
mechanics into an authoritative C11 core. The standards scope remains the same
as v2.0: the documented CCSDS Space Packet PDU profile, supported PUS-A/PUS-C
secondary-header layouts, and supported basic numeric CUC subset.

This file records direction only. The authoritative release checklist is
`V2_TRANSITION_ACCEPTANCE_LIST.md`; detailed performance, compliance, and
hardware evidence live in their dedicated documents rather than being copied
here.

## v2.1 architecture

- C11 caller-owned protocol core for packet/header/CRC/CUC/PUS/validation/stream primitives;
- compatible C++17 ownership/convenience layer;
- pure-C installed target and C-only build path;
- bounded transactional parsing and explicit caller-buffer APIs;
- standalone DAS/OpenOCD Cortex-M7 validation path;
- native arm64 installed-package validation.

## Standards invariant

Implementation refactoring must not change the release claim or wire semantics.
CCSDS/ECSS behavior remains guarded by independent fixed vectors, direct
negative fixtures, the complete PUS-C acknowledgement matrix, structured
validation evidence, sanitizer/fuzz gates, and physical/native target
validation. Round-trip behavior alone is not conformance evidence.

## Current v2.1 release work

Correctness and target qualification are complete:

- native regression/conformance suite: **134/134 PASS**;
- Raspberry Pi 5/native arm64: **PASS**;
- NUCLEO-H755ZI-Q/Cortex-M7: **PASS**;
- hosted Linux/Windows/Doxygen/robustness gates: established release gates.

Before v2.1.0 promotion/tagging:

1. complete issue #163, the broader matched v2.0-vs-v2.1 performance characterization;
2. finalize release/compliance documentation;
3. promote accepted `develop` to `main`;
4. require final `main` CI;
5. tag `v2.1.0`;
6. verify GitHub Release assets, package hashes, and GHCR images.

Performance measurements must remain subordinate to conformance: an optimization
that changes standards-defined bytes, accepted field ranges, tailoring rules, or
validation behavior is not acceptable merely because it benchmarks faster.

## Detailed evidence

- release gates: `V2_TRANSITION_ACCEPTANCE_LIST.md`;
- compliance statement: `COMPLIANCE.md`;
- CCSDS traceability: `CCSDS_COMPLIANCE.md`;
- PUS/CUC baseline: `docs/PUS_CUC_COMPLIANCE.md`;
- performance evidence: `docs/PERFORMANCE.md`;
- hardware evidence: `docs/V2_HARDWARE_VALIDATION.md`;
- robustness evidence: `docs/ROBUSTNESS.md`.
