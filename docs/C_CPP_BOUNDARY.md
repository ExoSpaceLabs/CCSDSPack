# C11 / C++17 boundary and validation ownership

Status: v2.2 development, deep source audit recorded 2026-11-13. See [v2.2 architecture audit](V2_2_ARCHITECTURE_AUDIT.md) for open blockers and evidence limits.

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
| CUC `time::serialize` / `deserialize` | C11 CUC codec | **Merged via PR #188:** removed duplicate C++ validation before C encoding. **Unmerged PR #201:** adds optional zero-allocation caller-owned C++ bounded encode and borrowed decode; retains the original owning vector overloads and error translation. The PUS TM timestamp setter and protected tail helpers use the bounded adapter, while built-in PUS-A/C TM codecs continue to encode/decode directly in C11. |
| PUS-A and PUS-C TM `serialize` | C11 PUS encode, including tailoring | **Merged via PR #188:** removed the duplicate C++ tailoring preflight; the C encoder checks it. |
| PUS identifier setters and tailoring width helper | C11 PUS identifier-width and fit helpers | **Merged via PR #188:** removed duplicated C++ bit arithmetic using `ccsds_pus_identifier_width_is_valid()` and `ccsds_pus_identifier_fits()`. Setter object state and error translation remain in C++. |
| PUS-A and PUS-C TM `deserialize` | C11 PUS decode, including tailoring | **PR #190 merged** with C++ diagnostic/error and transactionality tests. **Open A-01:** the C11 TM decoders themselves still modify output POD fields on late CUC failure. |
| Explicit PUS tailoring/config load | C11 checks protocol fields; C++ checks config schema | Public `validateTailoring` calls C11. Distinguish independent validation requests from duplicate preflight inside a codec operation. |
| `Header::setData` and primary-header setters | C11 `ccsds_primary_header_validate_ex` identifies first invalid field; C++ retains state transition and error mapping | **Implemented in PR #189**: removes C++ range constants; C++ maps the C11 field reason into the existing messages and keeps atomic assignment, INVALID recovery and IDLE transitions. |
| `Packet` contiguous/split parse | C11 packet_view / packet_parts | **Already consolidated** by PR #184. The C++ object factory and secondary-header selection stay C++. |
| `Validator` packet and PUS coherence | C11 validation core | C11 delegates are present; **A-04** identifies unsafe virtual-identity-based downcasts; **A-11** finds that Validator ignores configured Packet CRC16 parameters; **A-07** covers CRC temporary aggregation. Sequence validation advances validator state and must not be reused for read-only Manager inspection (**A-03**). |
| Manager segmentation, reassembly, stream | C11 segmentation/reassembly/stream core | C++ packet collection, application policy, indexing and ownership are legitimate. **A-03** read-side sequence mutations, **A-04** PUS type cast safety, **A-05** duplicated identifier bit packing and **A-08** packet-count narrowing remain open. |
| `DataField` capacity and registered headers | C++ object/container state | Capacity and custom-object checks correctly belong in C++, **but A-02** documents missing null checks and overflow-prone capacity addition in raw overloads. |
| CRC16 utility | C11 CRC | C++ wrapper delegates to C11; no second polynomial implementation. |

## Gate for future C/C++ boundary PRs

A PR touching a C11/C++ codec boundary must explain **which check has one
owner**, why any remaining C++ guard is object-specific, and test at least
one successful wire vector and each preserved error category. Reviewers
should reject independent reimplementations of C11 validation constants.

The [deep v2.2 audit](V2_2_ARCHITECTURE_AUDIT.md) found outstanding
correctness/safety issues in C11 and C++ despite green hosted PR CI.
Release freeze is blocked until A-01..A-04 and A-11 are resolved and verified.
Physical hardware qualification is an additional, later release gate.
