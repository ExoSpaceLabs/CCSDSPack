# CCSDSPack v2.0.0 Compliance Baseline

## Status

This document is the authoritative compliance baseline for CCSDSPack v2.0.0. It records the selected standards, the supported protocol scope, the current implementation status, and the evidence required before a compliance claim can be made.

The presence of an API or a successful internal encode/decode round trip is not, by itself, conformance evidence.

## Normative baseline

| Area | Selected baseline | v2.0.0 decision |
|---|---|---|
| CCSDS Space Packet Protocol | CCSDS 133.0-B-2, Issue 2, June 2020, including published editorial corrections applicable to that issue | Primary packet-layer target |
| Packet Utilisation Standard | ECSS-E-ST-70-41C, 15 April 2016 | Primary and only PUS revision targeted by v2.0.0 |
| Historical PUS-A | ECSS-E-70-41A, 30 January 2003 | Deferred; no v2.0.0 compliance claim |

Where this document and an implementation comment disagree, the normative standard and this baseline take precedence.

## PUS revision decision

- PUS-C means the revision defined by ECSS-E-ST-70-41C. It is not a generic variable-length secondary-header category.
- PUS revision and packet direction are independent properties.
- Telecommand and telemetry secondary headers are separate structures and shall not be interchangeable.
- PUS-A support is deferred from v2.0.0. Existing `PusA` behavior is legacy behavior until separately redesigned and verified.
- There is no standards revision named PUS-B in the selected baseline. Existing `PusB` behavior is non-compliant legacy behavior and shall be removed or renamed as proprietary before v2.0.0.
- Existing `PusC` behavior is not considered ECSS-E-ST-70-41C compliant merely because of its class name.

The standards-facing representation begins in `inc/CCSDSV2MissionProfile.h` with `PusRevision`, `PacketDirection`, and the mission-profile contract.

## Supported scope

CCSDSPack v2.0.0 targets:

- CCSDS Space Packets with the fixed six-octet primary header;
- packet creation, serialization, bounded parsing, and validation;
- segmented and unsegmented packet sequence-control fields;
- optional packet error control using no error-control field or CRC-16-CCITT;
- ECSS-E-ST-70-41C telecommand and telemetry packet secondary headers;
- mission-tailored identifiers and telemetry time fields;
- independent, externally verifiable golden-vector tests.

## Explicitly unsupported or deferred

The following are outside the v2.0.0 compliance claim:

- CCSDS transfer frames and virtual-channel processing;
- COP-1;
- CFDP;
- SpaceWire or any other transport binding;
- automatic reassembly of arbitrarily interleaved segmented streams;
- a C core, stable C ABI, or C wrapper architecture;
- complete implementation of every ECSS PUS service;
- mission-specific payload packet semantics;
- audio and video packet semantics;
- PUS-A compliance;
- the existing `PusB` wire format as a standards format.

## Status vocabulary

| Status | Meaning |
|---|---|
| Implemented | Behavior is present and matches independent conformance evidence. |
| Partially implemented | Some behavior exists, but one or more normative requirements or tests are missing. |
| Mission-tailored | The standard permits or requires mission-specific selection recorded by `MissionProfile`. |
| Unsupported | Intentionally outside the v2.0.0 scope. |
| Not applicable | Requirement does not apply to this library layer. |
| Non-compliant | Current behavior is known to conflict with the selected baseline. |
| Planned | No compliant implementation exists yet. |

## CCSDS Space Packet compliance matrix

The matrix is intentionally requirement-oriented rather than a claim that every sentence of the Blue Book has already been satisfied. Clause identifiers shall be refined against the controlled copy of CCSDS 133.0-B-2 as implementation PRs are completed.

| Requirement area | v2 requirement | Current status | Current implementation reference | Required evidence / target test |
|---|---|---|---|---|
| Primary header size | Primary header is exactly 6 octets | Partially implemented | `CCSDS::Header::serialize`, `CCSDS::Header::deserialize` | `test_v2_primary_header_exact_six_octets` |
| Packet version number | Version field is validated and encoded in 3 bits; v2 Space Packet profile uses version 0 | Non-compliant | `Header::setVersionNumber` silently masks values | `test_v2_reject_invalid_packet_version` |
| Packet type | Packet type is restricted to the defined one-bit values | Non-compliant | `Header::setType` silently masks values | `test_v2_reject_invalid_packet_type` |
| Secondary-header flag | Flag is encoded exactly and agrees with packet construction | Partially implemented | `Header`, `Packet::update` | `test_v2_secondary_header_flag_consistency` |
| APID | APID is an 11-bit value; normal APIDs 0..2046 are accepted and idle APID 2047 is handled explicitly | Non-compliant | Header storage is 16-bit, but configuration loads APID through `std::uint8_t` and setters mask values | `test_v2_apid_full_range_and_idle` |
| Packet sequence flags | Sequence flags are encoded in 2 bits and invalid public inputs fail | Non-compliant | `Header::setSequenceFlags` silently masks values | `test_v2_reject_invalid_sequence_flags` |
| Packet sequence count | Sequence count is 14 bits, preserved on parse, and may be non-zero for unsegmented packets | Non-compliant | `Packet::update` forces unsegmented count to zero; validator requires zero | `test_v2_unsegmented_nonzero_sequence_count` |
| Sequence rollover | Automatic counters roll over modulo 16384 | Planned | Current manager/counter behavior not accepted as evidence | `test_v2_sequence_count_rollover` |
| Independent counters | Higher-level automatic counters are maintained independently per APID; low-level codec remains stateless | Planned | Manager redesign required | `test_v2_independent_sequence_counters_per_apid` |
| Packet Data Length | Encoded value is the number of octets in the packet data field minus one | Non-compliant | `Packet::update` writes the serialized data-field size directly | `test_v2_packet_data_length_golden_vector` |
| Packet Data Length coverage | Secondary header, application data, and enabled packet error control are included | Non-compliant | Current CRC is outside the encoded length calculation | `test_v2_packet_data_length_includes_crc` |
| Maximum packet size | Length calculation prevents overflow and rejects unrepresentable packets | Planned | No accepted v2 validation path | `test_v2_reject_packet_too_large` |
| Packet boundary | Parser calculates total packet size as `6 + Packet Data Length + 1` | Non-compliant | `Packet::deserialize` copies all remaining bytes | `test_v2_parse_exact_packet_boundary` |
| Truncation handling | Truncated packets return a specific error without out-of-bounds access | Partially implemented | Minimum-size checks exist, but declared packet boundary is not enforced | `test_v2_reject_truncated_packet` |
| Concatenated packets | One parse consumes exactly one packet and reports consumed octets | Planned | Existing API does not report consumed length | `test_v2_parse_concatenated_packets` |
| Error-control presence | Packet error control is explicitly configured as `None` or `CRC16` | Non-compliant | Serialization and parsing assume a two-octet CRC | `test_v2_crc_disabled_packet` |
| CRC coverage | CRC covers the primary header, secondary header, and application data, ending before the CRC field | Non-compliant | `Packet::update` calculates CRC over the data field only | `test_v2_crc_complete_packet_golden_vector` |
| CRC encoding | CRC is serialized in network byte order | Partially implemented | `Packet::getCRCVectorBytes` writes MSB first | `test_v2_crc_big_endian` |
| CRC verification | Parser compares received and calculated CRC and returns a dedicated mismatch error | Non-compliant | Parser stores received CRC but does not verify it | `test_v2_crc_mismatch` |
| Read-only inspection | Inspection of a parsed packet does not mutate header, length, count, or CRC | Non-compliant | Multiple getters call `Packet::update` | `test_v2_parsed_packet_inspection_is_non_mutating` |
| Unknown extra bytes | Trailing bytes are not silently absorbed into a packet | Non-compliant | Existing deserialization consumes remaining input | `test_v2_trailing_bytes_not_consumed` |

## ECSS PUS-C traceability matrix

| Requirement area | v2 requirement | Classification | Current status | Required evidence / target test |
|---|---|---|---|---|
| Revision identity | PUS-C is represented explicitly as ECSS-E-ST-70-41C / version 2 | Mandatory | Contract added; codec planned | `test_v2_pus_revision_c_value` |
| Packet direction | TC and TM are modeled separately from revision | Mandatory | Contract added; concrete types planned | `test_v2_tc_tm_type_separation` |
| TC acknowledgement flags | PUS-C TC acknowledgement bits are represented and serialized | Mandatory for TC codec | Planned | `test_v2_pus_c_tc_ack_flags_vector` |
| Service type/subtype | Service type and subtype are represented in TC and TM headers | Mandatory | Legacy fields exist; compliant layout planned | Independent TC/TM vectors |
| Source identifier | Width is selected by mission profile | Mission-tailored | Profile contract added; validation/codec planned | `test_v2_source_id_widths` |
| Destination identifier | Width is selected by mission profile | Mission-tailored | Profile contract added; validation/codec planned | `test_v2_destination_id_widths` |
| TM time-reference status | Field is represented in PUS-C TM | Mandatory for TM codec | Planned | `test_v2_pus_c_tm_time_reference_status` |
| TM message-type counter | Field is represented in PUS-C TM | Mandatory for TM codec | Planned | `test_v2_pus_c_tm_message_type_counter` |
| TM timestamp presence | Presence is explicitly selected by mission profile | Mission-tailored | Profile contract added | `test_v2_tm_timestamp_presence_profile` |
| TM time format | CCSDS time family and total encoded length are explicit | Mission-tailored | Profile contract added; codec planned | CUC/CDS/CCS vectors as supported |
| Packet error control | Presence and algorithm are selected by mission profile | Mission-tailored | Profile contract added; packet integration planned | CRC and no-CRC PUS-C vectors |
| PUS services | A mission selects only the services and capability sets it implements | Mission-tailored | Documentation baseline only | Service-specific tests when implemented |

## Mission tailoring

The initial mission-tailoring rules, validation constraints, and defaults are defined in [`MISSION_TAILORING.md`](MISSION_TAILORING.md). A profile must be supplied to standards-facing v2 packet construction and parsing. Code shall not infer a profile from remaining bytes or from a legacy class name.

## Compliance evidence policy

A matrix row may be changed to **Implemented** only when all of the following exist:

1. a source-code reference;
2. a focused regression test;
3. an independent expected byte vector or independently calculated value where wire behavior is involved;
4. a negative test for rejected inputs where validation is involved;
5. no contradictory supported configuration path.

Internal serialization/deserialization round trips are useful regression tests, but they are not independent conformance evidence.

## Phase 0 decision record

Phase 0 is complete when this document and the mission-tailoring specification are merged, and the following decisions are treated as change-controlled:

- CCSDS 133.0-B-2 Issue 2 is the packet-layer baseline.
- ECSS-E-ST-70-41C is the primary and only PUS revision targeted by v2.0.0.
- PUS-A is deferred.
- PUS-B is rejected as a standards concept.
- TC and TM are separate directions, not PUS revisions.
- mission-tailored fields are explicit configuration, never parser guesswork.
- unsupported layers and services do not appear in v2 compliance claims.
