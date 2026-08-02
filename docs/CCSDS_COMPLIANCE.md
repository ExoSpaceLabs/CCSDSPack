# CCSDSPack v2.0.0 Compliance Baseline

## Status

This document defines the supported standards, inherited implementation evidence, remaining v2 work, and final claim boundary for CCSDSPack v2.0.0.

The v2 implementation baseline is the released `v1.2.0` state on `main`. Generic CCSDS packet behaviours validated for v1.2.0 are inherited by v2.0.0. They are not classified as missing merely because the original v2 planning documents were written before the v1.2.0 conformance work was completed.

Detailed v1.2.0 generic Space Packet traceability remains in the versioned root [`CCSDS_COMPLIANCE.md`](../CCSDS_COMPLIANCE.md). This document records the v2 delta, especially mission profiles and PUS-C.

## Normative baseline

| Area | Selected baseline | v2.0.0 decision |
|---|---|---|
| CCSDS Space Packet Protocol | CCSDS 133.0-B-2, Issue 2, including Editorial Change 2 of September 2024 | Space Packet PDU profile and applicable packet assembly/extraction behaviour |
| Packet Utilisation Standard | ECSS-E-ST-70-41C, 15 April 2016 | Sole PUS revision targeted by v2.0.0 |
| Historical PUS-A | ECSS-E-70-41A, 30 January 2003 | Deferred; no v2.0.0 implementation or compliance claim |

## Claim boundary

CCSDSPack v2.0.0 targets:

- construction, serialization, bounded parsing, inspection, and validation of CCSDS Space Packet PDUs;
- the fixed six-octet Packet Primary Header;
- the Packet Data Field, including optional secondary-header and mission-defined packet-error-control content;
- segmented and unsegmented Packet Sequence Count handling;
- one Packet Identification stream and one independent sequence counter per `CCSDS::Manager`;
- ECSS-E-ST-70-41C PUS-C telecommand and telemetry secondary headers;
- explicit mission-selected identifier widths and telemetry time configuration;
- independent generic CCSDS and PUS-C conformance evidence.

It does not claim:

- a complete implementation of every abstract CCSDS Packet Service or Octet String Service primitive;
- transfer frames, virtual channels, COP-1, CFDP, or a transport binding;
- complete implementation of every ECSS PUS service;
- PUS-A compliance;
- a C core or stable C ABI;
- automatic reassembly of arbitrarily interleaved segmented streams.

## PUS model decision

- PUS-C means the revision defined by ECSS-E-ST-70-41C.
- PUS revision and packet direction are independent properties.
- Telecommand and telemetry secondary headers are separate layouts and are not interchangeable.
- PUS-A is deferred from v2.0.0.
- There is no supported standards revision named PUS-B.
- Legacy v1 `PusA`, `PusB`, and `PusC` classes are project-specific formats and are removed from current v2 public/runtime code through #64 and #109.
- A matching class name is not evidence of conformance.

## Status vocabulary

| Status | Meaning |
|---|---|
| Inherited | Implemented and validated in the released v1.2.0 baseline |
| Complete | Implemented specifically for v2 and supported by evidence |
| Partial | Some implementation exists but v2 requirements or evidence remain |
| Pending | Required v2 implementation does not yet exist |
| Mission-tailored | Caller or mission configuration must select an allowed value |
| Deferred | Explicitly excluded from v2.0.0 |
| Unsupported | Outside the library claim |

## Generic CCSDS implementation status

| Capability | v2 status | Implementation/evidence |
|---|---|---|
| Six-octet primary header | Inherited | `CCSDS::Header`; v1.2 conformance tests |
| Packet Version Number zero enforcement | Inherited | generation, configuration, and parsing tests |
| Packet Type and Secondary Header Flag validation | Inherited | checked setters and parser validation |
| Full 11-bit APID range | Inherited | APIDs 0 through 2046 plus Idle APID 2047 |
| Idle Packet structural validation | Inherited | no secondary header and non-empty mission-defined idle data |
| Sequence Flags | Inherited | exact two-bit encoding and validation |
| Non-zero unsegmented sequence counts | Inherited | Packet and Manager regression tests |
| Modulo-16384 sequence advancement | Inherited | automatic and manual Manager modes |
| One Manager per Packet Identification stream | Inherited | complete identifier enforcement and transactional rejection |
| Packet Data Length | Inherited | Packet Data Field octets minus one |
| Maximum packet size | Inherited | complete 65,542-octet serialized range |
| Bounded packet parsing | Inherited | consumed-byte reporting, truncation rejection, trailing preservation |
| Transactional parsing | Inherited | failed parsing does not mutate prior packet state |
| Packet error control `None` | Inherited | explicit sender and receiver configuration |
| CRC16 mission profile | Inherited | correct coverage, big-endian encoding, parse-time validation |
| Non-mutating getters | Inherited | #70 regression evidence |
| Independent generic golden vectors | Inherited | #73 fixed vectors and independent generator |
| Generic negative/regression vectors | Inherited | #92 |
| Encoder, decoder, and validator generic semantics | Inherited | #93 CLI integration |
| Linux and Windows installed consumer | Inherited | v1.2 release gates |
| arm64 package and Cortex-M7 execution | Inherited | v1.2 hardware evidence |

### Manager sequence interpretation

CCSDSPack does not place a multi-APID counter map inside one Manager. One Manager is bound to one complete Packet Identification value and owns one sequence counter for that stream. Applications using several APIDs create several Manager instances or use independent Packet objects.

This model provides independent counters between APIDs while preserving the documented single-stream Manager abstraction.

## v2-specific implementation status

| Capability | Status | Issue(s) | Required evidence |
|---|---|---|---|
| Standards and tailoring baseline | Complete | #43, #44, #45, #91 | merged documents and public contract |
| v1.2.0 branch synchronization | In progress | #108 | mergeable PR and complete branch CI |
| Generic packet profile without PUS | Partial | #66 | profile tests and installed consumer |
| Mission-profile validation | Pending | #67 | positive and negative profile tests |
| Typed PUS revision and direction | Partial | #57 | public type and mismatch tests |
| Separate TC and TM abstractions | Pending | #58 | compile-time/runtime mismatch rejection |
| Structured secondary-header factory | Pending | #68 | fresh-instance and custom-header tests |
| Explicit error-returning finalization | Pending | #69 | finalization error tests |
| PUS-C TC secondary header | Pending | #59 | independent TC vectors |
| CCSDS time representation | Pending | #61 | encode/decode and invalid-time vectors |
| PUS-C TM secondary header | Pending | #60 | independent TM vectors with and without time |
| PUS/profile validator | Pending | #72 | structured failure tests |
| Legacy PusB removal | Pending | #64 | source/config/docs search evidence |
| Legacy PusA/PusC removal | Pending | #109 | source/config/docs search evidence |
| Independent PUS-C vectors | Pending | #74 | committed fixed vectors |
| PUS/profile negative vectors | Pending | #76 | deterministic expected failures |
| Sanitizer and fuzz testing | Pending | #77 | ASan, UBSan, and bounded fuzz CI |
| v2 configuration migration | Pending | #79 | generic, TC, and TM example configs |
| Manager profile integration | Pending | #80 | generation/parsing tests |
| CLI profile integration | Pending | #81, #82, #83 | independent CLI vector tests |
| Examples and diagrams | Pending | #84 | examples compile in CI |
| Migration guide | Pending | #85 | before/after API and configuration examples |
| README and claim update | Pending | #86 | release-facing review |
| v2 package and CI gates | Partial | #87 | all v2 release jobs pass |
| v2.0.0 release | Pending | #88 | final release gate |

## Mission tailoring

The active tailoring contract is defined in [`MISSION_TAILORING.md`](MISSION_TAILORING.md) and `inc/CCSDSMissionProfile.h`.

A default profile represents generic CCSDS and does not silently enable PUS. A PUS-C profile explicitly selects revision, direction, identifier widths, packet error control, and telemetry time configuration.

Code shall not infer:

- packet-error-control presence from trailing bytes;
- timestamp format from remaining packet length;
- packet direction from a class name;
- unknown secondary-header bytes as PUS-C;
- PUS enablement from default object construction.

## PUS-C conformance requirements

### Telecommand

The supported TC codec shall verify:

- reserved-bit value;
- encoded PUS version;
- each acknowledgement flag;
- service type and subtype;
- mission-selected source-ID width;
- direction compatibility with the primary-header Packet Type;
- absence of legacy custom application-data-length fields.

### Telemetry

The supported TM codec shall verify:

- reserved-bit value;
- encoded PUS version;
- time-reference status;
- service type and subtype;
- message-type counter;
- mission-selected destination-ID width;
- optional timestamp presence and exact encoded size;
- direction compatibility with the primary-header Packet Type;
- absence of legacy custom application-data-length fields.

### Time

Only time-code formats with implemented deterministic codecs, documented epoch/P-field policy, and independent vectors may appear in the final v2.0.0 claim. Merely listing CUC, CDS, or CCS in an enum is not implementation evidence.

## Required final evidence

Before v2.0.0 is released, the repository shall contain:

- inherited generic CCSDS vectors and regressions passing unchanged;
- independent PUS-C TC vectors;
- independent PUS-C TM vectors with and without timestamp;
- identifier-width and reserved-bit vectors;
- invalid-profile, wrong-direction, malformed-time, CRC, boundary, and truncation vectors;
- Linux and Windows native tests;
- Doxygen validation;
- installed-package consumer tests;
- encoder, decoder, and validator integration tests;
- ASan and UBSan jobs;
- bounded parser fuzz smoke tests;
- arm64 package validation;
- Cortex-M7 validation including representative PUS-C TC and TM packets.

## Release restrictions

The v2.0.0 release claim is prohibited while any of the following remains true:

- proper PUS-C TC or TM codecs lack independent vectors;
- profile validation permits ambiguous parsing;
- current public/runtime code exposes legacy `PusA`, `PusB`, or `PusC` classes;
- PUS-A is implied to be supported;
- sanitizer, fuzz, Linux, Windows, CLI, installed-consumer, package, arm64, or MCU release gates fail;
- README, migration guide, release notes, and this compliance document disagree with the implementation.
