# v2.2 development plan

[Documentation index](README.md) | [Architecture](ARCHITECTURE.md) | [Deep v2.2 audit](V2_2_ARCHITECTURE_AUDIT.md) | [C11 API](C_API.md) | [v2.1 hardware evidence](V2_HARDWARE_VALIDATION.md)

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
| P1 | [#182: eliminate duplicated split-buffer packet validation](https://github.com/ExoSpaceLabs/CCSDSPack/issues/182) | **Merged into `develop` via PR #184** (`844b2fa`), issue closed. Unified C11 contiguous/split policy, retained C++ diagnostics; split CRC allocations reduced 7→5. No-CRC path ~9–10% slower in matched hosted run. See [PERFORMANCE.md](PERFORMANCE.md); physical real-target acceptance remains a future release gate. |
| P1 | [#166: optimize CUC C++ wrapper](https://github.com/ExoSpaceLabs/CCSDSPack/issues/166) | **Merged to `develop` via PR #202** (focused PR #201, issue #166 closed): optional C++17 bounded, caller-owned CUC encode/borrowed decode with the same C11 codec, existing owning API retained, and PUS-A/C TM `setTimestamp()` successful validation without temporary vector. Same-process Ubuntu 26.04: bounded encode 13.339 ns/0 alloc vs owning encode 28.980 ns/1 alloc; PUS-A/C setter ~15.25 ns/0 alloc. Legacy owning encode is unchanged; no packet-wide speedup claim. Negative vectors, 64-layout parity and transactionality tests added. CI and measured, scoped performance were accepted on `develop` SHA `aefadba71fd682545fbf5403f75d1cf6ad5fafe9`; the owning-vector API retains its old cost. See [PERFORMANCE.md](PERFORMANCE.md). |
| P1 | [#187: C11/C++ protocol validation ownership](https://github.com/ExoSpaceLabs/CCSDSPack/issues/187) | **Six source-level P1 defects merged to `develop` via PR #202**: C11 PUS TM failure-transactionality (#193/A-01); DataField null/overflow guards (#194/A-02); per-Packet CRC16 Validator (#195/A-11); read-only Manager validation (#196/A-03); RTTI-free PUS subtype safety (#197/A-04); and Packet mutable-alias/finalization coherence (#199/A-13). The six-fix combination was hosted-qualified separately in draft PR #200; final PR-head and post-merge Linux/Windows/robustness CI PASS, with 166/166 C++ unit tests on Ubuntu 24.04. Parent issue #187 stays open for remaining A-05/A-06/A-07/A-08/A-09/A-10/A-12 dispositions, documentation audit, release acceptance and hardware evidence. See [deep architecture audit](V2_2_ARCHITECTURE_AUDIT.md). |
| P2 | Optional C11 packet-processing context | **Defer** until EXN OBC consumers show a real need beyond existing C packet view, sequence validation, PUS codecs and reassembly; no second owning Manager. |
| Maintenance | [#176: retire obsolete v1 test configuration](https://github.com/ExoSpaceLabs/CCSDSPack/issues/176) | **Merged via PR #185** (`b0a55f0`), issue closed. Retired unused PVN=1 config and installed tester fixtures now explicit. All native/CLI/package/hosted gates passed. |
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

1. **Completed:** [PR #202](https://github.com/ExoSpaceLabs/CCSDSPack/pull/202) integrated all seven focused P1/CUC changes into `develop` at `aefadba71fd682545fbf5403f75d1cf6ad5fafe9`. PR-head and post-merge Linux/Windows/robustness CI passed. **Next:** resolve outstanding A-05/A-06/A-07/A-08/A-09/A-10/A-12 architecture/API/documentation dispositions under [#187](https://github.com/ExoSpaceLabs/CCSDSPack/issues/187), and complete matched release-performance and compliance evidence on the eventual frozen candidate. CUC issue #166 is closed for the approved dual-API design. Freeze only the **exact qualified source SHA** after these gates.
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

## C11/C++17 validation ownership

The library-wide [boundary audit](C_CPP_BOUNDARY.md) defines **C11 as the sole
protocol-rule authority** and C++ as the owning object/adaptation layer.
PRs #188, #189 and #190 are merged into `develop`: duplicate CUC/PUS TM encode
and PUS TM decode preflights have been removed, and primary-header setters
now use the C11 field-reason validator. Historical C++ error diagnostics and
transactional failure handling have regression coverage. PR #202 subsequently integrated the six P1 source-level fixes and the
bounded CUC/PUS TM timestamp optimization; the exact merged source passed
hosted CI with 166/166 C++ tests. The [2026-11-13 deep
audit](V2_2_ARCHITECTURE_AUDIT.md) preserves the original findings and
now records their integration disposition. Remaining P2 protocol-policy,
configuration, documentation and CRC allocation work stays open under #187.
CUC #166 is closed for the approved dual-API performance decision.
Passing hosted CI and MCU cross-builds does **not** constitute physical
v2.2 qualification.
