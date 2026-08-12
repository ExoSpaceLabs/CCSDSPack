# CCSDSPack v2 robustness validation

## Purpose

This document defines the automated parser-robustness evidence used by the v2.0.0 release. It complements semantic malformed-vector tests and structured `ccsds::Validator` checks; fuzzing is not used as a substitute for protocol conformance vectors.

## Sanitizer gates

The `Robustness` GitHub Actions workflow runs the complete native regression/conformance suite in two dedicated Clang builds:

- AddressSanitizer (ASan), with leak detection and immediate failure on detected memory errors;
- UndefinedBehaviorSanitizer (UBSan), with immediate failure and stack traces on detected undefined behavior.

These jobs instrument the CCSDSPack shared library and the native tester. They are release gates rather than optional local diagnostics.

## Bounded fuzz targets

The same workflow builds four libFuzzer targets with ASan and UBSan enabled:

| Target | Surface exercised |
|---|---|
| `ccsdspack_fuzz_declared_packet_size` | six-octet primary-header parsing and raw declared-packet-size inspection |
| `ccsdspack_fuzz_packet` | generic pointer-plus-size bounded Packet parsing with both supported packet error-control modes |
| `ccsdspack_fuzz_pus` | typed raw bounded parsing for PUS-A TC/TM and PUS-C TC/TM with valid optional tailoring combinations |
| `ccsdspack_fuzz_cuc` | CUC configuration validation, decode, and successful decode/encode round-trip behavior |

CI executes each target for a bounded 2,500 generated inputs, with a 4,096-byte input limit, five-second per-input timeout, and 512 MiB RSS limit. These limits make the smoke gate deterministic enough for pull requests while still exercising malformed declared sizes, field combinations, PUS layouts, raw-buffer adapters, and CUC state under memory-safety instrumentation.

## Assertions

Successful bounded Packet parses must never report more consumed bytes than were supplied and must agree with the packet size declared by the primary header. Successful CUC decoding must re-encode to the same complete encoded value. Sanitizer findings, libFuzzer crashes, timeouts, or RSS-limit violations fail CI.

## Evidence boundary

The v2.0.0 Packet and PUS object model remains vector-backed and is not claimed to be globally heap-free or zero-copy. The RSS bound is a robustness guard against pathological behavior in the bounded smoke corpus, not a proof that parsing performs no allocation.

Fuzz smoke coverage is also not an exhaustive proof that no malformed byte string can fail. Release confidence comes from the combination of independent fixed vectors, direct malformed-vector tests, structured Validator checks, ASan/UBSan regression execution, bounded fuzzing, installed-consumer tests, and target validation.
