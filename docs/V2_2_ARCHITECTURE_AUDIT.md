# v2.2 deep architecture and acceptance audit

**Audit date:** 2026-11-13  
**Baseline:** `develop` after PR #191, merge commit `8cbdd9e33cf9d8a33027b81872bbe67001bcc925`.  
**Verdict:** **NOT READY** for implementation freeze, performance sign-off, or physical v2.2 release qualification.

## Evidence discipline and scope

This is a **source/documentation audit**, not a test execution. Inspected current
`develop` CMake/build profiles; public C/C++ headers; C11 primary header,
packet encode/view, PUS-A/C TC/TM, CUC, validation, segmentation, reassembly,
stream/walking and incremental receiver; C++ Packet, Header, DataField, Manager,
Validator, PUS headers/factory, configuration path; related C and C++ regression
sources; project architecture, boundary, compliance, profile, hardware evidence,
release notes, roadmap and performance documentation.

PR #190's updated head passed Linux/Windows, ASan/UBSan/fuzz, Doxygen and
package/cross-build CI before merge. PR #191's documentation head passed the
same hosted workflow categories. Those CI results do **not** execute the
specific newly identified negative cases below; the present audit did not run
local compilers or hardware. Physical v2.1 evidence applies only to its
recorded source/build/harness identities, not this v2.2 tree.

**Standards versus implementation:** The standards baseline is CCSDS
133.0-B-2 Issue 2 + Editorial Change 2 (notably 4.1.3.1, 4.1.3.4,
4.1.3.5), CCSDS 301.0-B-4 for supported basic numeric CUC, ECSS-E-70-41A
for PUS-A, and ECSS-E-ST-70-41C for PUS-C. The library implements a
documented subset; neither this audit nor the hosted CI confers blanket
CCSDS/ECSS compliance. The optional CRC16 trailer is CCSDSPack packet-level
policy within the CCSDS data field. The four-byte sync marker belongs to
external stream framing.

## Architectural classification

| Boundary | Classification | Evidence / caveat |
| --- | --- | --- |
| C-only C11 core and MCU build | **Coherent** | CMake uses C11 core as separate target, supports `CCSDSPACK_BUILD_CPP=OFF`, and builds C++17 optionally. MCU mode excludes hosted configuration/CLI. Static review found no heap-ownership machinery in the C11 protocol modules. |
| Primary header | **Coherent** | PR #189's C11 `ccsds_primary_header_validate_ex()` is the field-width decision source. C++ Header owns INVALID/IDLE state and historic error text. Space Packet PVN=000 policy applies at packet level, not to the standalone three-bit width check. |
| Packet parse | **Coherent** | `packet_view.c::parse_parts()` is the shared C11 contiguous/split policy; the C++ Packet adapters use it, then stage owned state before commit. Typed PUS, opaque and custom header selection remains C++ object policy. |
| Packet encode | **Partially duplicated** | C11 `packet_finalize_parts` / `packet_encode_parts` own version/idle/length rules. `Packet.cpp::validatePacketForSerialization()` also checks version and idle constraints for historic diagnostics. Cross-object secondary-header presence and direction checks belong in C++ unless C11 receives sufficient metadata. |
| PUS TC/TM codecs | **Coherent dispatch, defect in C11 TM failure state** | C++ delegates wire encode/decode to PUS C11 functions; PUS-A and PUS-C remain distinct. Invalid CUC on C11 TM decoding can partially update output (A-01). |
| CUC | **Single wire implementation** | `time::serialize/deserialize` call C11. `TmSecondaryHeader::setTimestamp()` uses C++ allocating serialization just to validate a prospective field. This is C11-backed but expensive, not independent wire logic. |
| Validator | **Correct source of decisions, allocation/typing risks** | Calls `ccsds_validate_packet_coherence`, `ccsds_validate_pus_coherence`, and C11 CUC/CRC utilities. CRC input is assembled via temporary vectors; PUS subtype discrimination relies on unchecked static downcasts (A-04). Its report itself is fixed capacity. |
| Manager | **C11 protocol core leveraged, read semantics defective** | Uses C11 segmentation, sequence increments, stream walker and reassembly. C++ owns packet storage/contracts. Public getters directly advance the persistent validator (A-03). Manager also independently packs the primary-header Packet Identification bits. |
| DataField/custom header | **Object-level rules valid, unsafe raw overload** | Registry checks, serialized object sizes and configured capacity are legitimate C++ rules. Pointer-size typed overload lacks a null guard; sum-based capacity checks can overflow (A-02). |
| Stream primitives | **Coherent by inspection** | `ccsds_packet_stream_t` walks a buffer; `ccsds_stream_receiver_t` incrementally frames sync-prefixed data using caller-owned scratch. External timeout/resync remains mission/transport policy; no hardware or timing guarantee implied. |
| Deployment/release | **Not qualified for v2.2** | Host CI and cross-build evidence exist for PR heads. Native Pi 5, physical H755, frozen-hash documentation and final release review are outstanding. |

## Findings requiring disposition

Severity means release impact under CCSDSPack's transactional, bounded and
RTTI-free promises. A *confirmed source defect* follows directly from
observable control flow; runtime reproduction remains a required acceptance
step before closing it.

### A-01 | P1/blocker | C11 PUS TM decode modifies output on failure

**Code:** `src/c/pus_tm.c`, `ccsds_pus_a_tm_decode()`,
`ccsds_pus_c_tm_decode()`.

Both functions assign service/identifier/counter fields to `fields_out`
**before** invoking `ccsds_cuc_decode(...,&fields_out->timestamp)`.
With valid tailoring and valid-length bytes but a mismatched explicit CUC
P-field, CUC decode returns `CCSDS_STATUS_CUC_PFIELD_MISMATCH` after
the earlier fields have changed. This violates the library's bounded,
transactional C parse contract. PR #190 tested the C++ wrapper using
local temporary fields; that test does not cover the public C11 output.

**Required:** decode into a local zero-initialized `ccsds_pus_*_tm_fields_t`
and publish `*fields_out` only on complete success. Add direct C11
sentinel-structure tests for PUS-A and PUS-C with explicit-P-field mismatch;
assert identical output bytes on failure, then a fixed valid vector and
disabled-timestamp zeroing.

### A-02 | P1/blocker | C++ raw secondary-header pointer and arithmetic

**Code:** `src/CCSDSDataField.cpp`,
`DataField::setSecondaryHeader(const uint8_t*,size_t,const string&)`
and `DataField::setApplicationData(const uint8_t*,size_t)`.

The typed pointer overload constructs
`std::vector<uint8_t>(pData,pData+sizeData)` without a null guard,
even though the untyped pointer overload does guard. Nonzero size with a
null pointer can invoke undefined behavior. Additionally, comparisons using
`sizeData + secondaryHeaderSize` or
`sizeData + m_applicationData.size()` can wrap on `size_t`,
so caller-controlled size is not safely bounded before pointer arithmetic.

**Required:** reject null pointer before arithmetic; enforce maximum using
subtraction/ordered comparisons against `m_dataPacketSize`, and preserve
existing error codes/messages where possible. Cover null (zero and nonzero
size), `SIZE_MAX` with a small nonnull buffer (must reject before access),
exact capacity, and one-byte overflow in C++ raw API tests under ASan/UBSan.

### A-03 | P1/blocker | Manager getters mutate sequence-validator state

**Code:** `src/CCSDSManager.cpp`:
`getPacketBufferAtIndex()`, `getApplicationDataBuffer()`.

Both call `m_validator.validate(packet)` on the persistent validator.
`Validator::validate()` calls `acceptSequence()` after a valid packet,
which advances `m_sequenceState`. Consequently, reading an index twice
or reconstructing the same stored packet sequence twice can reject the
second call with an unexpected sequence count. A failed multi-packet read
can also leave earlier accepted sequence-state transitions committed.
`getApplicationDataBuffer()` separately calls C11
`ccsds_reassembly_accept()` with its own sequence validator: this repeats
sequence validation as well.

**Required:** specify the distinction between stream *ingestion*
(validation state advances) and non-mutating *inspection/reconstruction*.
Use an independent validator with a fresh sequence state for readback,
without weakening template/PUS/CRC checks, or introduce a documented
read-only validation operation. Preserve the atomic load/add workflow.
Test repeated reads, random index access, reconstruction after partial
failure, and sequential ingest with rollover.

### A-04 | P1/safety risk | PUS identity is not a type-safe downcast proof

**Code:** `src/PusSecondaryHeaderFactory.cpp::clone()`,
`src/CCSDSValidator.cpp::coreTailoringValid()`,
`src/CCSDSManager.cpp::sameSecondaryContract()`,
`src/CCSDSPacket.cpp::deserializeBoundedWithSecondaryHeader()`.

The public `SecondaryHeaderAbstract::isPusHeader()` is virtual and may
be overridden by user-defined headers. Public PUS `TcSecondaryHeader`/
`TmSecondaryHeader` bases can also be subclassed with caller-supplied
revision/direction. Several code paths use `static_cast` to a specific
PUS class based **only** on virtual identity values. Those values do
not establish C++ dynamic type. A misclassified custom object can
therefore enter undefined-behavior territory in RTTI-free builds.
This is a **reachable API design risk**, not a reproduced crash.

**Required:** make PUS downcast dispatch type-safe without requiring RTTI
(e.g., constrained clone/descriptor virtual dispatch implemented only by the
four final concrete classes), or provably reject nonconcrete PUS-like
objects at public entry points. Add an adversarial custom-header regression
under UBSan and preserve valid PUS-A/C parsing/template parity. Document the
extension contract explicitly.

### A-05 | P2/architecture | Remaining C++ wire-rule repetition

**Code:** `Packet.cpp::validatePacketForSerialization()` (PVN,
Idle Packet), `Packet::setSequenceCount()`,
`Manager::setSequenceCount()` (14-bit limit),
`Manager::packetIdentifier()` (field pack),
`TcSecondaryHeader::setAcknowledgementFlags()` (four bits),
`rev_c::TmHeader::setTimeReferenceStatus()` (four bits), and PUS hosted
configuration fixed-width coercion.

**Disposition:** For every check, separate **C++ object consistency**
from **C11 wire policy**. Retain object state/diagnostics and config-schema
checking in C++; delegate protocol constants to C11 validator helpers or
the existing C11 codec. Do not replace clear cheap field validators with
heap-allocating full encode calls. Add exact error-message regression tests
for prospective setters. PUS fixed-width checks in the Validator also
duplicate C11 policy and need one authority.

### A-06 | P2/contract decision | C11 TM encode can partially write before error

**Code:** `src/c/pus_tm.c`, both TM encoders.

The encoder writes header/identifier bytes before calling CUC encode.
A coarse/fine overflow discovered in the timestamp codec returns an error
with output partially modified. `written_out` is not committed, which
helps, but the output buffer itself is not failure-atomic. The current C
API should explicitly define whether output bytes are unspecified on error;
if strong transactionality is required, prevalidate CUC counter fits
before writing. Add an output sentinel test either way.

### A-07 | P2/performance | Avoidable validation and allocation work

**Code:** `src/CCSDSValidator.cpp::validate()` concatenates a serialized
primary-header vector and full data-field vector solely to calculate CRC.
`TmSecondaryHeader::setTimestamp()` allocates a C++ CUC serialization
buffer to validate numeric range. C11 PUS TM decode validates tailoring
and then calls `*_encoded_size()`, which validates it again.

**Disposition:** Stream CRC over existing pieces with C11 CRC update,
expose a nonencoding CUC value-fit check if beneficial, and only optimize
the C11 duplicate-validation path with measured evidence. No
regression/performance improvement is asserted here; issue #166 is separate.

### A-08 | P2/API consistency | Large Manager packet counts narrow to 16 bits

**Code:** `Manager::getTotalPackets()` returns
`static_cast<uint16_t>(m_packets.size())` although the owning vector and C11
segmentation plan use `size_t`. More than 65535 stored packets yields a
truncated count. This is distinct from the deliberately **documented**
saturating `Packet::getFullPacketLength()`.

**Required:** decide whether to limit stored packet count or introduce an
additive full-width accessor without silently breaking the public API.
Add a boundary test at a practical reduced-limit abstraction.

### A-09 | P2/documentation | Historical v2.1 release text is stale

**Evidence:** PR #178 is merged into `main`, and `v2.1.0` resolves as
an existing GitHub tag/ref. Nevertheless `CHANGELOG.md` calls v2.1.0
"pending release tag"; `COMPLIANCE.md`, `RELEASE_NOTES.md`,
`CCSDS_COMPLIANCE.md`, `docs/PUS_CUC_COMPLIANCE.md` and
`docs/CCSDS_133_0_B_2_PROFILE.md` contain pre-promotion or earlier
candidate wording. Independently verifying every Release asset and GHCR
image is outside this audit. The v2.1 physical acceptance in
`docs/V2_HARDWARE_VALIDATION.md` is historical and must remain attributed
to commit `8d6fe89`.

**Required:** reconcile current versus historical statements. Do not erase
old source identities, test results or published qualification records.
Avoid presenting a v2.1 candidate as still awaiting a merge that happened.

### A-10 | P2/API contract | Hosted config loads modify objects before error

**Code:** `Packet::loadFromConfig()`, PUS header `loadFromConfig()` methods.

Several config properties are committed before subsequent keys are
validated, so an unsuccessful load can leave partially updated state.
The repository explicitly promises transactional **wire parsing**,
not necessarily transactional config updates. Decide and document whether
config loads should be atomic; if so, stage into a temporary instance
and add invalid-late-key regression tests.

## Important distinctions: what is *not* a defect by itself

- The owning C++ `Packet` and `Manager` can allocate; only C11
  caller-buffer primitives are expected to avoid hidden heap ownership.
- C++ secondary-header registration, `DataField` capacity, object/PUS
  direction matching and typed factory selection are **cross-object policy**,
  not automatically duplicated C11 byte checks.
- A C++ prospective setter may validate using the C11 codec/helper. The
  objection is duplicating protocol constants or allocating solely to check.
- C++ `Header` supports the 3-bit version field independently; packet
  parsing/serialization enforces the supported Space Packet version.
- `getFullPacketLength()` saturating at `UINT16_MAX` is an established,
  explicitly documented compatibility accessor; use `getSerializedSize()`
  for the actual packet length.
- Sync framing is not a CCSDS Space Packet primary-header field. The
  incremental receiver intentionally relies on a caller-provided timeout
  for plausible but incomplete frames.

## Required acceptance campaign

1. **Correctness before performance:** resolve A-01, A-02, A-03 and
   A-04; settle A-06's C output-failure contract. Add fixed C11 malformed
   vectors, C++ raw-pointer negative tests, Manager repeat-read tests and
   RTTI-free adversarial PUS-subtype tests.
2. **Complete the one-authority inventory:** disposition A-05 and A-07;
   ensure every repeated wire rule either delegates to C11 or is documented
   as an object/config constraint. Re-test legacy error codes/messages.
3. **Resolve API semantics:** decide A-08 and A-10, with additive interfaces
   preferred over breaking changes.
4. **Document coherently:** correct A-09 without rewriting historical
   qualification evidence; update `docs/C_CPP_BOUNDARY.md`,
   `docs/ARCHITECTURE.md`, compliance matrices and evidence mapping as
   implementation changes land. Keep standards scope conservative.
5. **Close architecture issue #187 only after review evidence:** all required
   code PRs merged, static audit re-run, C11/C++ parity and negative tests
   passed, and Linux/Windows, C-only installed consumer, package and
   MCU/no-exception/no-RTTI cross-build plus ASan/UBSan/fuzz/Doxygen PASS
   on the final implementation SHA.
6. **Then** address measured CUC performance issue #166 without changing wire
   behavior. Decide to optimize, accept or defer using matched measurements.
7. **Only after gates 1-6:** freeze the v2.2 implementation, reconcile
   release documentation/version/changelog, run native Raspberry Pi 5 arm64
   and physical STM32H755 M7 qualification against the same source/build
   identities, promote through reviewed `develop -> main`, and verify
   automatic tagging/publication. A cross-build alone is not hardware evidence.

**Release acceptance remains BLOCKED.** No v2.2 physical PASS or current
commit test execution is claimed by this audit.
