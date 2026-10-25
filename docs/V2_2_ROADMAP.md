# v2.2 development plan

[Documentation index](README.md) | [Architecture](ARCHITECTURE.md) | [C11 API](C_API.md) | [v2.1 hardware evidence](V2_HARDWARE_VALIDATION.md)

**Status:** development on `develop`; **v2.1.0 remains the current tagged,
physically qualified release on `main`**. Neither the 2.1.0 tag nor its
published packages should be rewritten to incorporate the features below.
`VERSION` is updated as part of a dedicated v2.2 release candidate, not by
ad-hoc code commits that could trigger premature automatic tagging.

## Architectural rule

The C11 core owns protocol wire behavior and bounded, caller-buffer operations.
The optional C++17 layer owns containers, object lifetimes, registration of
secondary-header types, typed convenience APIs, hosted config and CLI. Moving
object ownership to C would add duplication without improving correctness.
Consolidate any remaining duplicated protocol *decisions* instead.

## Development work

| Priority | Item | Status / scope |
|---|---|---|
| P0 | [#38: incremental sync-framed C receiver](https://github.com/ExoSpaceLabs/CCSDSPack/issues/38) | **Merged** via PR #181 (commit `04fabc8c`), C11, no allocation; Linux/Windows/ASan/UBSan/fuzz/Doxygen plus C-only installed consumer PASS. |
| P1 | [#182: eliminate duplicated split-buffer packet validation](https://github.com/ExoSpaceLabs/CCSDSPack/issues/182) | **Hosted validation completed in PR #184**. Unified C11 contiguous/split policy, retained C++ diagnostics; split CRC allocations reduced 7→5. No-CRC path ~9–10% slower in matched hosted run. See [PERFORMANCE.md](PERFORMANCE.md); real-target acceptance is a later release gate. |
| P1 | [#166: optimize CUC C++ wrapper](https://github.com/ExoSpaceLabs/CCSDSPack/issues/166) | **Open**. Profile C vs C++ and compare before/after on one runner; correctness alone is insufficient to accept an optimization. Experimental PR #183 was **closed without merging** after a same-runner comparison with frozen v2.1 measured 38.19 → 38.92 ns/op for CUC encode with unchanged allocations; it demonstrated no speedup. Keep issue open for direct C-vs-wrapper profiling and repeated paired benchmarks. |
| P2 | Optional C11 packet-processing context | **Defer** until EXN OBC consumers show a real need beyond existing C packet view, sequence validation, PUS codecs and reassembly; no second owning Manager. |
| Maintenance | [#176: retire obsolete v1 test configuration](https://github.com/ExoSpaceLabs/CCSDSPack/issues/176) | **Open**. Run package/CLI tests if changing installed fixtures. |
| Future | CFDP #39–#41, Python #37 | **Outside the v2.2 core migration** unless separately scoped and justified. |

## Stream recovery contract

The new `ccsds_stream_receiver_t` is the **sync-prefixed, incremental**
counterpart of `ccsds_packet_stream_t` (contiguous-buffer cursor).
A sync-like sequence within a valid packet payload must never preempt the
packet. Oversized headers, invalid packets and CRC failures are rejected with
bounded suffix rescanning. A *plausibly long but incomplete* packet requires
an external timeout and an explicit resync call; the library does not own
the MCU timer, RTOS thread, DMA or transport queue. See
[STREAM_RECEIVER.md](STREAM_RECEIVER.md) for contract and sample code.

## Qualification before v2.2 publication

1. Audit the exact `develop` implementation, library headers, build flags,
   documentation and issue dispositions, and freeze its source SHA.
2. Pass Linux, Windows, Doxygen, C-only installed-package/consumer,
   ASan/UBSan, bounded fuzz and cross-package builds on that SHA.
3. Requalify a **native Raspberry Pi 5 arm64 package** and physically run
   **NUCLEO-H755ZI-Q Cortex-M7** acceptance against the same implementation;
   capture source/DEB/MCU library/ELF hashes, UART transcript and OpenOCD verification.
   Use a released DAS tag only in the STM32 hardware validation harness, not
   as a CCSDSPack library dependency.
4. Review any changes against the frozen implementation; accept only
   evidence-only documentation commits after the hardware run.
5. Promote via reviewed `develop -> main` PR, verify final `main` CI,
   automatically tag the explicit `VERSION=2.2.0` candidate, and verify
   GitHub Release assets and GHCR publication. Do not tag from feature branches.

These are **future release gates**, not claims of physical validation for
post-2.1 features.
