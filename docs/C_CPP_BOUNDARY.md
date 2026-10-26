# C11 / C++17 boundary and validation ownership

Status: v2.2 development, 2026-10-26.

## Non-negotiable ownership rule

**The C11 protocol core is the single authority for wire-protocol validation.**
The C++17 facade may manage `std::vector`, exceptions/results, configuration
objects, custom user-defined header objects, and lifetime, but must **not**
independently implement CCSDS/PUS range, CRC, packet-length, framing, CUC
width/epoch, or other wire rules.

- **C11:** validates raw spans and protocol fields, performs encode/decode,
  CRC, segmentation, reassembly, and streaming. Caller supplies buffers,
  capacities, output lengths and state. No `malloc`, C++ containers, STL,
  ownership transfer, filesystem, or runtime-dependent allocation.
- **C++:** converts typed inputs to C PODs, allocates/owns return objects,
  selects registered custom header implementations, maps C error statuses,
  enforces object-lifetime and application-specific constraints, and handles
  configuration syntax/types.
- One `serialize()` or `deserialize()` operation should call the C11
  codec's authoritative validation **once**. A separate explicit `validate()`
  method may intentionally invoke a C11 validation API as its *own operation*.
  A separate object setter may validate a prospective object state; it must
  delegate any protocol rule to C11, not duplicate numeric constants.
- No "optimization" may bypass C11 validation, weaken wire requirements, or
  silently alter public error codes/messages. New C status/reason values may
  be needed to preserve specific facade diagnostics.
- Native C consumers, MCU cross-builds, fuzz, sanitizer, C++ negative tests
  and matched benchmarks must continue to pass across refactors.

## Boundary audit

| API boundary | Rule ownership | Current classification / action |
| --- | --- | --- |
| CUC `time::serialize` / `deserialize` | C11 CUC codec | **This change:** remove C++ prevalidation before C encode. Decode already delegates directly. C++ still owns output vector and error translation. |
| PUS-A and PUS-C TM `serialize` | C11 PUS encode, including tailoring | **This change:** remove the duplicate C++ tailoring preflight; the C encoder checks it. |
| PUS identifier setters and tailoring width helper | C11 PUS identifier-width and fit helpers | **This change:** remove duplicated C++ bit arithmetic using `ccsds_pus_identifier_width_is_valid()` and `ccsds_pus_identifier_fits()`. Setter object state and error translation remain in C++. |
| PUS-A and PUS-C TM `deserialize` | C11 PUS decode, including tailoring | **Follow-up:** remove redundant `validateTailoring` preflight after verifying historical C++ error diagnostics and error precedence, including invalid timestamp configuration. |
| Explicit PUS tailoring/config load | C11 checks protocol fields; C++ checks config schema | Public `validateTailoring` calls C11. Distinguish independent validation requests from duplicate preflight inside a codec operation. |
| `Header::setData` and primary-header setters | C11 primary-header field validator; C++ state transition | **Follow-up:** migrate repeated `0x07` / `0x01` / APID / sequence-width tests to a C11 diagnostic validator while keeping the exact C++ setter error mapping and invalid-state semantics. |
| `Packet` contiguous/split parse | C11 packet_view / packet_parts | **Already consolidated** by PR #184. The C++ object factory and secondary-header selection stay C++. |
| `Validator` packet and PUS coherence | C11 validation core | Already delegates coherence checks. Review C++ CRC temporary aggregation and C++-only formatting separately; avoid repeated CRC or serialization of the same packet solely for validation. |
| Manager segmentation, reassembly, stream | C11 segmentation/reassembly/stream core | C++ packet collection, application policy, indexing, and ownership constraints legitimately remain in C++. |
| `DataField` capacity and registered headers | C++ object/container state | These checks concern the owning object's configured memory budget or custom header instances, not protocol wire-format duplication. |
| CRC16 utility | C11 CRC | C++ wrapper delegates to C11; no second polynomial implementation. |

## Gate for future C/C++ boundary PRs

A PR touching a C11/C++ codec boundary must explain **which check has one
owner**, why any remaining C++ guard is object-specific, and test at least
one successful wire vector and each preserved error category. Reviewers
should reject independent reimplementations of C11 validation constants.

The audit is intentionally explicit about remaining work. Passing hosted
tests does **not** mean the entire historical facade has already been
refactored. Hardware qualification is still a separate v2.2 release gate.
