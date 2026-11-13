# v2.2 deep architecture and acceptance audit

**Audit date:** 2026-11-13  
**Baseline:** `develop` after PR #191, merge commit `8cbdd9e33cf9d8a33027b81872bbe67001bcc925`.  
**Verdict:** **NOT READY** for implementation freeze, performance sign-off, or physical v2.2 release qualification.

## Post-audit integration disposition (PR #202)

The findings and source snippets below record the **original pre-fix
`develop` baseline**, not the post-integration state. Integration PR
[#202](https://github.com/ExoSpaceLabs/CCSDSPack/pull/202) squash-merged the seven
individually qualified fixes into `develop` at
`aefadba71fd682545fbf5403f75d1cf6ad5fafe9`. This qualification does
not by itself confer release acceptance or physical-target coverage.

| Finding | Integrated change and evidence |
| --- | --- |
| A-01 | C11 PUS-A/C TM decode publishes output only after successful CUC checks; failure-sentinel and fixed-vector tests from #193. |
| A-02 | Null/overflow-safe C++ DataField raw-buffer handling and boundary tests from #194. |
| A-03 | Stateless Manager readback validation, with sequential ingest behavior retained, from #196. |
| A-04 | RTTI-free typed PUS dispatch and adversarial identity tests from #197. |
| A-11 | Validator checks Packet-specific CRC16 parameters and associated fixed vectors from #195. |
| A-13 | Packet mutable-alias aware finalization, guarded CRC cache and callback-free read snapshot from #199; custom user callbacks remain outside rollback guarantees. |
| A-07 (CUC/PUS part) | #201 retains both public C++ ownership models, delegates to the C11 CUC codec, removes the temporary vector in successful TM timestamp setters and protected TM tail helpers; tests and same-process benchmark evidence in [PERFORMANCE.md](PERFORMANCE.md). |

The independent six-fix combination was qualified on the draft
[#200](https://github.com/ExoSpaceLabs/CCSDSPack/pull/200) at
`5712e7b68a1d9909210cfa509bd351ecd5331006`. CUC/PUS change #201
was independently qualified at `7f02ba902eedc8b32200e45ba9c3b01f2cd557ea`.
These source trees were reconciled in PR #202. The **exact PR head**
`52bae8e151fed97036255cc44ae947037000f9f5` passed Linux
[43452871782](https://github.com/ExoSpaceLabs/CCSDSPack/actions/runs/43452871782),
Windows [43452871820](https://github.com/ExoSpaceLabs/CCSDSPack/actions/runs/43452871820),
ASan/UBSan/fuzz [43452871885](https://github.com/ExoSpaceLabs/CCSDSPack/actions/runs/43452871885),
Doxygen [43452872102](https://github.com/ExoSpaceLabs/CCSDSPack/actions/runs/43452872102),
standalone C11/installed consumers and H755 firmware cross-build.
Post-merge `develop` Linux [43453537008](https://github.com/ExoSpaceLabs/CCSDSPack/actions/runs/43453537008),
Windows [43453536922](https://github.com/ExoSpaceLabs/CCSDSPack/actions/runs/43453536922),
and robustness [43453536808](https://github.com/ExoSpaceLabs/CCSDSPack/actions/runs/43453536808)
also **PASS**, including **166/166 C++ unit tests** on Ubuntu 24.04.
This is hosted/cross-build evidence, not native Pi 5 or physical STM32H755
acceptance. Issues #166 and #198 are closed for their approved, scoped
implementation; parent architecture issue #187 stays open.

**Still open:** A-05 duplicated C++ protocol checks; A-06 C11 TM encode
output-on-failure contract; remaining A-07 Validator CRC allocations and
C11 redundant tailoring checks; A-08 Manager count narrowing; A-10 partial
hosted configuration mutation; A-12 C/C++ data-field capacity discrepancy;
documentation audit A-09; and the v2.2 hardware/release gates. Keep parent
issue #187 open pending these dispositions and the final integration audit.

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

## Architectural classification (original audited baseline)

| Boundary | Classification | Evidence / caveat |
| --- | --- | --- |
| C-only C11 core and MCU build | **Coherent** | CMake uses C11 core as separate target, supports `CCSDSPACK_BUILD_CPP=OFF`, and builds C++17 optionally. MCU mode excludes hosted configuration/CLI. Static review found no heap-ownership machinery in the C11 protocol modules. |
| Primary header | **Coherent** | PR #189's C11 `ccsds_primary_header_validate_ex()` is the field-width decision source. C++ Header owns INVALID/IDLE state and historic error text. Space Packet PVN=000 policy applies at packet level, not to the standalone three-bit width check. |
| Packet parse | **Coherent** | `packet_view.c::parse_parts()` is the shared C11 contiguous/split policy; the C++ Packet adapters use it, then stage owned state before commit. Typed PUS, opaque and custom header selection remains C++ object policy. |
| Packet encode | **Partially duplicated** | C11 `packet_finalize_parts` / `packet_encode_parts` own version/idle/length rules. `Packet.cpp::validatePacketForSerialization()` also checks version and idle constraints for historic diagnostics. Cross-object secondary-header presence and direction checks belong in C++ unless C11 receives sufficient metadata. |
| PUS TC/TM codecs | **Coherent dispatch, defect in C11 TM failure state** | C++ delegates wire encode/decode to PUS C11 functions; PUS-A and PUS-C remain distinct. Invalid CUC on C11 TM decoding can partially update output (A-01). |
| CUC | **Single wire implementation** | `time::serialize/deserialize` call C11. In the original audited baseline, `TmSecondaryHeader::setTimestamp()` allocates a temporary CUC vector to validate a prospective field; PR #201 (squash-integrated via merged #202) replaces it with stack-bounded CUC validation and adds caller-owned C++ overloads without duplicating wire rules. |
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

### A-11 | P1/blocker | Validator ignores Packet's configured CRC16 parameters

**Code:** `src/CCSDSValidator.cpp::Validator::validate()` (~line 146),
`inc/CCSDSPacket.h::setCrcConfig()`, `src/CCSDSPacket.cpp` finalization.

`Packet::setCrcConfig(polynomial, initial, finalXor)` is a supported public
API. Packet finalization and parsing pass `m_CRC16Config` into the C11
core. However, the Validator constructs a **default**
`const CRC16Config crcConfig{}` to check every CRC16 packet. A correctly
serialized packet using non-default parameters can therefore report
`ValidationCode::Crc16` failure. The current
`testGroupEdgeCases.cpp` explicitly tests a non-default wire vector
(initial value 0x0000), but `testGroupValidator.cpp` has no matching
configured-CRC parity test.

**Required:** expose the Packet's CRC configuration through a const accessor
or a bounded C11 CRC validation adapter, and use the *actual Packet* settings
in `Validator`. Prefer incremental C11 CRC update over constructing two
temporary owning vectors (A-07). Add positive default/non-default
`Packet::serialize -> Validator::validate -> Packet::deserialize` parity
tests and negative corruption tests that use the same explicit configuration.
Verify error codes and Manager validation with the custom profile.

### A-13 | P1/blocker | Packet update/serialization cache coherency and custom callback side effects

**Code:** `src/CCSDSPacket.cpp::update()/serialize()`, public mutable
`Packet::getPrimaryHeader()/getDataField()/getSecondaryHeader()`,
`src/CCSDSDataField.cpp::update()`, and `Manager` const/readback output.

A cached `m_updateStatus` could remain true after callers mutated a Header,
application bytes or shared secondary header through public references. The
next serialization could reuse a stale CRC16 despite emitting changed bytes.
In disabled-auto-update mode a same-length modification could likewise emit
a stale trailer. `Manager::getPacketsBuffer() const` serialized shallow Packet
copies, which could invoke a mutating user callback on a shared header.

**Remediation under review:** [issue #198](https://github.com/ExoSpaceLabs/CCSDSPack/issues/198)
and [PR #199](https://github.com/ExoSpaceLabs/CCSDSPack/pull/199).
Normal serialization re-finalizes using C11 when a mutable subobject has
escaped or any secondary header is present. It retains CRC caching only for
unaliased, secondary-header-free Packets with setter-based invalidation.
Disabled-auto-update preserves finalized wire bytes only if CRC/length still
match a callback-free independent C11 finalization. Explicit `update()`
always finalizes. A const callback-free `serializeSnapshot()` is used for
Manager read-style output so copied shared headers are not mutated.

**Remaining acceptance:** independently exercise C11 CRC validation on direct
Header/DataField/PUS mutations, stale-manual checks, guarded cache reuse,
callback size-growth and bounded buffer handling, repeatable const Manager
readback, Linux/Windows, ASan/UBSan/fuzz, C-only and MCU cross-builds. The
legacy third-party `void SecondaryHeaderAbstract::update(DataField*)`
callback remains intentionally stateful/non-transactional; arbitrary custom
callbacks cannot be retrofitted with rollback guarantees. Explicitly
document this limitation rather than claiming atomic user-code effects.
Benchmark cached and alias-sensitive CRC paths; the initial always-recompute
prototype exhibited a prohibitive large-buffer regression, motivating the
guarded cache. This architecture problem is a **v2.2 release gate** and
must be resolved before #166 or hardware freeze. Source review and PR CI
are not physical validation.

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
In the original audited `develop` baseline, `TmSecondaryHeader::setTimestamp()`
created an owning CUC vector solely to validate numeric range. C11 PUS TM decode validates
tailoring and then calls `*_encoded_size()`, which validates it again.

**Disposition:** **Partially addressed by merged PR #202** (focused #201): C++17 bounded
CUC encode/decode overloads call the same C11 codec; successful PUS-A/C TM
`setTimestamp()` operations no longer allocate. The protected TM tail
helpers also stop materializing intermediate CUC vectors. Fixed-vector,
overflow/error message and transactional state tests protect behavior.
The owning vector API remains supported, and built-in PUS-A/C TM wire
codecs already delegate to C11. Follow-up CRC aggregation and C11
duplicate-tailoring validation remain open, subject to profiling. See
issue #166 and the matched candidate evidence in
[PERFORMANCE.md](PERFORMANCE.md). No whole-packet speedup is claimed.

### A-08 | P2/API consistency | Large Manager packet counts narrow to 16 bits

**Code:** `Manager::getTotalPackets()` returns
`static_cast<uint16_t>(m_packets.size())` although the owning vector and C11
segmentation plan use `size_t`. More than 65535 stored packets yields a
truncated count. This is distinct from the deliberately **documented**
saturating `Packet::getFullPacketLength()`.

**Required:** decide whether to limit stored packet count or introduce an
additive full-width accessor without silently breaking the public API.
Add a boundary test at a practical reduced-limit abstraction.

### A-12 | P2/API/documentation | C++ data-field capacity is narrower than the C11 wire limit

**Code:** `inc/CCSDSDataField.h` (`m_dataPacketSize` is
`uint16_t`), `DataField::setApplicationData()`,
`Packet::getSerializedSize()` and `src/c/packet_encode.c`.

CCSDS Packet Data Length can represent up to **65,536** octets after the
primary header (CCSDS 133.0-B-2 Issue 2 + EC2, 4.1.3.5).
The C11 packet codec supports that maximum, but the C++ DataField storage
budget is a `uint16_t` capped at 65,535 *secondary/application* bytes.
With CRC disabled, the C++ owning API therefore cannot generate the legal
maximum 65,536-byte data field, despite the generic PDU profile describing
the full CCSDS range. With CRC enabled, the two trailer bytes count toward
the wire Packet Data Field and must be included in the 65,536-byte limit;
the budget currently does not directly reserve those two bytes.

**Required:** explicitly distinguish the normative wire range, C11 core
capability and C++ configured capacity. Either preserve the intentional
C++ limit and document it, or widen C++ capacity/accounting in a
separately reviewed compatibility change. Test exact last-successful
sizes with both PEC modes and the next invalid size. Avoid wrapping
the 16-bit legacy size accessors.

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

**Documentation resolution on audit branch:** the changelog, release notes,
concise/detailed compliance statements, PUS/CUC baseline and Space Packet
profile now distinguish verified PR #178/tag completion from unverified
Release asset/GHCR publication. Prior 2026-10-25 hardware identities remain
unchanged. Recheck on merged `develop` before marking A-09 fully closed.

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

1. **Correctness before performance:** resolve A-01, A-02, A-03,
   A-04, A-11 and A-13; settle A-06's C output-failure contract. Add fixed C11 malformed
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
6. **Completed for the approved dual-API CUC design:** issue #166 was closed
   after measured, bounded C++ encoding and PUS TM setter optimization landed via
   #202. Legacy owning-vector performance is unchanged; review broader
   release performance separately. Do not alter CUC wire requirements.
7. **Only after gates 1-6:** freeze the v2.2 implementation, reconcile
   release documentation/version/changelog, run native Raspberry Pi 5 arm64
   and physical STM32H755 M7 qualification against the same source/build
   identities, promote through reviewed `develop -> main`, and verify
   automatic tagging/publication. A cross-build alone is not hardware evidence.

**Release acceptance remains BLOCKED.** No v2.2 physical PASS or current
commit test execution is claimed by this audit.
