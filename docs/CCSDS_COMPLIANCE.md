# CCSDSPack v2.0.0 Compliance Baseline

## Status

This document is the authoritative compliance baseline for CCSDSPack v2.0.0. It records:

- the selected normative standards;
- the supported protocol and library scope;
- clause-level traceability to CCSDS 133.0-B-2;
- the current implementation status;
- the evidence required before a compliance claim can be made.

The presence of an API, a successful internal encode/decode round trip, or a matching class name is not, by itself, conformance evidence.

## Normative baseline

| Area | Selected baseline | v2.0.0 decision |
|---|---|---|
| CCSDS Space Packet Protocol | CCSDS 133.0-B-2, Recommended Standard Issue 2, June 2020, including Editorial Change 1 of October 2020 and Editorial Change 2 of September 2024 | Packet PDU, applicable packet assembly/extraction behaviour, and managed packet-format parameters |
| Packet Utilisation Standard | ECSS-E-ST-70-41C, 15 April 2016 | Primary and only PUS revision targeted by v2.0.0 |
| Historical PUS-A | ECSS-E-70-41A, 30 January 2003 | Deferred; no v2.0.0 compliance claim |

Where this document and an implementation comment disagree, the normative standard and this baseline take precedence.

## Conformance claim boundary

CCSDSPack v2.0.0 targets conformance for the supported Space Packet PDU and the library behaviours required to create, serialize, parse, inspect, and validate that PDU.

It does **not** currently claim to be a complete CCSDS Space Packet Protocol entity implementing every abstract service primitive and every sending-, receiving-, or transport-side function listed by the Protocol Implementation Conformance Statement in annex A of CCSDS 133.0-B-2.

In particular:

- packet-format and codec requirements are in scope;
- packet assembly and packet extraction behaviour used by the library are in scope;
- lower-layer packet transfer is outside the library scope;
- complete abstract `PACKET` and `OCTET_STRING` service primitive APIs are outside the v2.0.0 compliance claim;
- network routing, transfer frames, virtual channels, and transport bindings are outside the v2.0.0 compliance claim.

A future claim of complete CCSDS 133.0-B-2 protocol-entity conformance would require a completed PICS covering every mandatory annex A item.

## PUS revision decision

- PUS-C means the revision defined by ECSS-E-ST-70-41C. It is not a generic variable-length secondary-header category.
- PUS revision and packet direction are independent properties.
- Telecommand and telemetry secondary headers are separate structures and shall not be interchangeable.
- PUS-A support is deferred from v2.0.0. Existing `PusA` behaviour is legacy behaviour until separately redesigned and verified.
- There is no standards revision named PUS-B in the selected baseline. Existing `PusB` behaviour is non-compliant legacy behaviour and shall be removed or renamed as proprietary before v2.0.0.
- Existing `PusC` behaviour is not considered ECSS-E-ST-70-41C compliant merely because of its class name.

The standards-facing representation begins in `inc/CCSDSMissionProfile.h` with `PusRevision`, `PacketDirection`, and the mission-profile contract.

## Supported scope

CCSDSPack v2.0.0 targets:

- CCSDS Space Packets with the fixed six-octet Packet Primary Header;
- packet creation, serialization, bounded parsing, and validation;
- segmented and unsegmented packet sequence-control fields;
- independent Packet Sequence Counts per APID in the higher-level manager;
- explicit packet error-control selection through the mission profile;
- ECSS-E-ST-70-41C telecommand and telemetry secondary headers;
- mission-tailored identifiers and telemetry time fields;
- independent, externally verifiable golden-vector tests.

## Explicitly unsupported or deferred

The following are outside the v2.0.0 compliance claim:

- CCSDS transfer frames and virtual-channel processing;
- COP-1;
- CFDP;
- SpaceWire or any other transport binding;
- complete implementation of CCSDS Packet Service and Octet String Service primitives;
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
| Implemented | Behaviour is present and matches independent conformance evidence. |
| Partially implemented | Some behaviour exists, but one or more normative requirements or tests are missing. |
| Mission-tailored | The standard permits or requires mission-specific selection recorded by `MissionProfile`. |
| Unsupported | Intentionally outside the v2.0.0 compliance claim. |
| Not applicable | The requirement does not apply to the supported library layer. |
| Non-compliant | Current behaviour is known to conflict with the selected baseline. |
| Planned | No compliant implementation exists yet. |

## Traceability conventions

The CCSDS clause references below identify the normative source of each requirement.

The following markers are used:

- **Direct**: the row corresponds directly to one or more normative CCSDS requirements.
- **Derived**: the row is an implementation requirement necessary to preserve a directly specified CCSDS property, such as self-delimitation or an exact length count.
- **External**: the behaviour is not specified by CCSDS 133.0-B-2 and is governed by another selected standard or by the mission profile.
- **Library invariant**: the behaviour is required to keep the public library API deterministic and non-mutating but is not itself a CCSDS wire-format clause.

Annex A PICS item identifiers are included where the CCSDS PICS proforma groups the corresponding capability.

## Annex A PICS scope summary

This table records the relationship between the CCSDS 133.0-B-2 PICS and the v2.0.0 library claim. It is a scope statement, not a completed system-level PICS.

| PICS item(s) | CCSDS reference | Capability | v2.0.0 classification | Current status |
|---|---|---|---|---|
| SPP-1 | 3.2.2 | Space Packet service data unit | In scope through the `Packet` representation, not through a complete abstract service API | Partially implemented |
| SPP-2 | 3.2.3 | Octet String service data unit | Outside the complete-service-API claim | Unsupported |
| SPP-3, SPP-7 | 3.3.2.2, 3.4.2.2 | APID service parameter | In scope | Non-compliant |
| SPP-4, SPP-9 | 3.3.2.3, 3.4.2.4 | Packet/Data Loss Indicator | Outside the v2.0.0 claim | Unsupported |
| SPP-5 | 3.3.2.4 | Quality-of-Service requirement | Outside the v2.0.0 claim | Unsupported |
| SPP-6 | 3.4.2.1 | Octet String parameter | Represented only as packet data, not as a complete service primitive | Partially implemented |
| SPP-8 | 3.4.2.3 | Secondary Header Indicator | In scope through the Secondary Header Flag and mission profile | Partially implemented |
| SPP-10 to SPP-13 | 3.3.3.2, 3.3.3.3, 3.4.3.2, 3.4.3.3 | Abstract Packet and Octet String service primitives | Outside the v2.0.0 claim | Unsupported |
| SPP-14 | 4.1 | Space Packet PDU | In scope | Partially implemented |
| SPP-15 | 4.1.3 | Packet Primary Header | In scope | Non-compliant |
| SPP-16 | 4.1.4 | Packet Data Field | In scope | Partially implemented |
| SPP-17 | 4.1.4.2 | Packet Secondary Header | Conditional and mission-tailored; in scope where enabled | Partially implemented |
| SPP-18 | 4.1.4.3 | User Data Field | Conditional; in scope | Partially implemented |
| SPP-19 | 4.2.2 | Packet Assembly Function | In scope where performed by the library | Non-compliant |
| SPP-20 | 4.2.3 | Packet Transfer Function | Lower-layer transport responsibility | Not applicable |
| SPP-21 | 4.3.2 | Packet Extraction Function | In scope where performed by the parser | Non-compliant |
| SPP-22 | 4.3.3 | Packet Reception Function | APID-based routing may be provided by higher-level code; complete subnetwork reception is outside scope | Partially implemented |
| SPP-23 | table 5-1 | Maximum Packet Length | In scope | Planned |
| SPP-24 | table 5-1 | Packet Type of outgoing packets | In scope | Partially implemented |
| SPP-25 | table 5-1 | Packet Multiplexing Scheme | Mission-specific transport/queueing policy | Unsupported |
| SPP-26 | table 5-1 | Service Type per APID | Outside the complete-service-API claim; may be represented later by configuration | Unsupported |

## CCSDS Space Packet clause traceability matrix

| ID | CCSDS 133.0-B-2 clause(s) | PICS item(s) | Trace type | Requirement | Current status | Current implementation reference | Required evidence / target test |
|---|---|---|---|---|---|---|---|
| SPP-PDU-001 | 4.1.2.1 | SPP-14, SPP-15, SPP-16 | Direct | A Space Packet contains a contiguous six-octet Packet Primary Header followed by a mandatory Packet Data Field of 1 to 65536 octets. | Partially implemented | `CCSDS::Header`, `CCSDS::Packet` | `test_v2_space_packet_structure` |
| SPP-PDU-002 | 4.1.2.2 | SPP-14, SPP-23 | Direct | Total Space Packet size is from 7 to 65542 octets, subject to any smaller configured implementation limit. | Planned | No accepted v2 maximum-size validation path | `test_v2_packet_size_boundaries` |
| SPP-HDR-001 | 4.1.3.1 | SPP-15 | Direct | The Packet Primary Header is mandatory, exactly six octets, and contains the Packet Version Number, Packet Identification Field, Packet Sequence Control Field, and Packet Data Length in that order. | Partially implemented | `CCSDS::Header::serialize`, `CCSDS::Header::deserialize` | `test_v2_primary_header_exact_six_octets` |
| SPP-HDR-002 | 4.1.3.2.1, 4.1.3.2.2 | SPP-15 | Direct | Bits 0-2 contain the Packet Version Number and shall be `000` for a Space Packet defined by CCSDS 133.0-B-2. | Non-compliant | `Header::setVersionNumber` silently masks values | `test_v2_reject_invalid_packet_version` |
| SPP-HDR-003 | 4.1.3.3.1.1, 4.1.3.3.1.2 | SPP-15 | Direct | Bits 3-15 form the Packet Identification Field containing Packet Type, Secondary Header Flag, and APID. | Partially implemented | `CCSDS::Header` | `test_v2_packet_identification_field_layout` |
| SPP-HDR-004 | 4.1.3.3.2.1 to 4.1.3.3.2.3 | SPP-15, SPP-24 | Direct | Bit 3 contains Packet Type: `0` for telemetry/reporting and `1` for telecommand/requesting. | Non-compliant | `Header::setType` silently masks values | `test_v2_reject_invalid_packet_type` |
| SPP-HDR-005 | 4.1.3.3.3.1 to 4.1.3.3.3.4; 4.1.4.2.1.2 | SPP-8, SPP-15, SPP-17 | Direct | Bit 4 signals the actual presence of the Packet Secondary Header; it is static for an APID and managed data path during a Mission Phase and shall be `0` for Idle Packets. | Partially implemented | `Header`, `Packet::update`, secondary-header factory | `test_v2_secondary_header_flag_consistency`; `test_v2_idle_packet_has_no_secondary_header` |
| SPP-HDR-006 | 4.1.3.3.4.1 to 4.1.3.3.4.4 | SPP-3, SPP-7, SPP-15 | Direct | Bits 5-15 contain the 11-bit APID. APIDs 0-2046 are available for mission use; APID 2047 identifies an Idle Packet. Values above 2047 are invalid. | Non-compliant | Header storage is 16-bit, but configuration loads APID through `std::uint8_t`; public setters silently mask values | `test_v2_apid_full_range_and_idle`; `test_v2_reject_apid_above_2047` |
| SPP-HDR-007 | 4.1.3.4.1.1, 4.1.3.4.1.2 | SPP-15 | Direct | Bits 16-31 form the Packet Sequence Control Field containing two Sequence Flag bits and a 14-bit Packet Sequence Count or Packet Name. | Partially implemented | `CCSDS::Header` | `test_v2_sequence_control_field_layout` |
| SPP-HDR-008 | 4.1.3.4.2.1 to 4.1.3.4.2.3 | SPP-15 | Direct | Sequence Flags encode continuation `00`, first `01`, last `10`, and unsegmented `11`; Octet String service packets must use `11`. | Non-compliant | `Header::setSequenceFlags` silently masks values | `test_v2_sequence_flag_vectors`; `test_v2_reject_invalid_sequence_flags` |
| SPP-HDR-009 | 4.1.3.4.3.1, 4.1.3.4.3.2 | SPP-15 | Direct | Bits 18-31 contain the Packet Sequence Count for telemetry and either Packet Sequence Count or Packet Name for telecommand. Parsed values are preserved exactly. | Non-compliant | `Packet::update` forces unsegmented Packet Sequence Count to zero | `test_v2_unsegmented_nonzero_sequence_count`; `test_v2_preserve_parsed_sequence_count` |
| SPP-HDR-010 | 4.1.3.4.3.3 | SPP-15, SPP-19 | Direct | Packet Sequence Counts are unique and independent for each user application identified by APID and are not shared across APIDs. | Planned | Manager redesign required | `test_v2_independent_sequence_counters_per_apid` |
| SPP-HDR-011 | 4.1.3.4.3.4; 4.2.2.4 | SPP-19 | Direct | Automatic Packet Sequence Counts are continuous modulo 16384; the packet assembly path maintains and applies the sequence counter. | Planned | Current counter behaviour is not accepted as v2 evidence | `test_v2_sequence_count_rollover` |
| SPP-HDR-012 | 4.1.3.5.1 to 4.1.3.5.3 | SPP-15 | Direct | Bits 32-47 contain Packet Data Length, encoded as the total number of octets in the Packet Data Field minus one. | Non-compliant | `Packet::update` writes the serialized data-field size directly | `test_v2_packet_data_length_golden_vector` |
| SPP-DATA-001 | 4.1.4.1.1, 4.1.4.1.2 | SPP-16, SPP-17, SPP-18 | Direct | The Packet Data Field contains at least one octet and consists of the optional Packet Secondary Header followed by the optional User Data Field, with at least one of them present. | Partially implemented | `DataField`, secondary-header support, application-data storage | `test_v2_packet_data_field_composition`; `test_v2_reject_empty_packet_data_field` |
| SPP-DATA-002 | 4.1.4.2.1.1 to 4.1.4.2.1.4 | SPP-17 | Direct | If present, the Packet Secondary Header immediately follows the Packet Primary Header, occupies an integral number of octets, and has mission-managed contents known at both ends. | Partially implemented | Legacy secondary-header classes and factory | `test_v2_secondary_header_position`; profile-specific vectors |
| SPP-DATA-003 | 4.1.4.2.1.5, 4.1.4.2.1.6 | SPP-17 | Direct | The generic CCSDS Packet Secondary Header contains a time field, ancillary data, or both, and the selected form remains static for a managed data path. ECSS PUS-C supplies the selected secondary-header format for the v2 PUS profile. | Mission-tailored | `CCSDS::MissionProfile`; compliant codecs planned | Profile validation and PUS-C TC/TM vectors |
| SPP-LEN-001 | 4.1.3.5.2, 4.1.3.5.3; 4.1.4.1.1 | SPP-15, SPP-16 | Direct | Every octet following the Packet Primary Header and belonging to the Space Packet contributes to Packet Data Length, including all enabled secondary-header, user-data, and externally defined packet-error-control octets. | Non-compliant | Current CRC bytes are outside the encoded length calculation | `test_v2_packet_data_length_includes_crc` |
| SPP-LEN-002 | 4.1.2.2; 4.1.3.5.2, 4.1.3.5.3 | SPP-14, SPP-23 | Direct | Construction rejects packet-data sizes that cannot be represented by the 16-bit minus-one Packet Data Length field and prevents arithmetic overflow. | Planned | No accepted v2 validation path | `test_v2_reject_packet_too_large`; overflow boundary tests |
| SPP-PARSE-001 | 2.1.1; 4.1.2.1; 4.1.3.5.2, 4.1.3.5.3 | SPP-14, SPP-21 | Derived | A parser determines one exact packet boundary as `6 + Packet Data Length + 1` octets. | Non-compliant | `Packet::deserialize` consumes all remaining input | `test_v2_parse_exact_packet_boundary` |
| SPP-PARSE-002 | 4.1.2.1, 4.1.2.2; 4.1.3.5.2, 4.1.3.5.3; 4.3.2.2 | SPP-14, SPP-21 | Derived | Input shorter than the declared packet boundary is rejected with a specific truncation error and without an out-of-bounds read. | Partially implemented | Minimum-size checks exist, but the declared packet boundary is not enforced | `test_v2_reject_truncated_packet` |
| SPP-PARSE-003 | 2.1.1; 4.1.3.5.2, 4.1.3.5.3; 4.3.2.2 | SPP-14, SPP-21 | Derived | A parser consumes exactly one self-delimited Space Packet and reports the consumed octet count so concatenated packets can be parsed individually. | Planned | Existing API does not report consumed length | `test_v2_parse_concatenated_packets` |
| SPP-PARSE-004 | 2.1.1; 4.1.3.5.2, 4.1.3.5.3 | SPP-14, SPP-21 | Derived | Bytes after the declared boundary are not silently absorbed into the current Space Packet. | Non-compliant | Existing deserialization consumes all trailing input | `test_v2_trailing_bytes_not_consumed` |
| SPP-PROC-001 | 4.2.2.2 to 4.2.2.4 | SPP-19 | Direct | Packet assembly generates the Packet Primary Header, maps the Secondary Header Indicator to the Secondary Header Flag, and applies the maintained Packet Sequence Count. | Non-compliant | `Packet::update`, manager construction path | `test_v2_packet_assembly_fields` |
| SPP-PROC-002 | 4.3.2.1, 4.3.2.2 | SPP-21 | Direct | Packet extraction removes the Packet Primary Header, exposes whether a Packet Secondary Header is present, and can use Packet Sequence Count continuity to identify packet loss. | Planned | Parser exposes packet components but does not implement the complete extraction behaviour | `test_v2_packet_extraction`; loss-indicator work only if retained in scope |
| SPP-PROC-003 | 4.3.3.1 to 4.3.3.3 | SPP-22 | Direct | Receiving-side code may demultiplex received Space Packets by APID and delivers Packet Service packets intact. | Partially implemented | Higher-level manager behaviour requires redesign and explicit scope | `test_v2_apid_demultiplexing` if retained in scope |
| SPP-MGMT-001 | 5.1; 5.2 table 5-1 | SPP-23, SPP-24, SPP-26 | Direct | Maximum Packet Length, outgoing Packet Type, service type per APID, and Packet Secondary Header contents are explicit managed configuration rather than parser guesses. | Partially implemented | `CCSDS::MissionProfile`; legacy configuration paths remain | Mission-profile and configuration migration tests |
| SPP-API-001 | No direct CCSDS clause | N/A | Library invariant | Inspection of a parsed packet does not mutate its header fields, length, sequence count, data, or error-control value. | Non-compliant | Multiple getters call `Packet::update` | `test_v2_parsed_packet_inspection_is_non_mutating` |

## Packet error-control assumptions

CCSDS 133.0-B-2 does not define a Packet Error Control field or a CRC algorithm for the Space Packet PDU.

For CCSDSPack v2.0.0:

- packet error-control presence and algorithm are selected explicitly by the mission profile;
- ECSS-E-ST-70-41C governs packet-error-control behaviour for the supported PUS-C profile;
- no CRC is inferred from the number of trailing bytes;
- when enabled, packet-error-control octets are part of the Packet Data Field and therefore contribute to the Packet Data Length under 4.1.3.5;
- CRC coverage, byte order, and validation require independent PUS-C reference vectors.

| Requirement area | CCSDS 133.0-B-2 relationship | Classification | Current status | Required evidence / target test |
|---|---|---|---|---|
| Error-control presence | Not defined by CCSDS 133.0-B-2; externally selected by the mission profile. Its encoded octets are included in Packet Data Length under 4.1.3.5. | External / mission-tailored | Non-compliant | `test_v2_crc_disabled_packet`; `test_v2_packet_data_length_includes_crc` |
| CRC coverage | Not defined by CCSDS 133.0-B-2; governed by ECSS-E-ST-70-41C for PUS-C. | External / mission-tailored | Non-compliant | `test_v2_crc_complete_packet_golden_vector` |
| CRC encoding | Not defined by CCSDS 133.0-B-2; governed by the selected external profile. | External / mission-tailored | Partially implemented | `test_v2_crc_big_endian` |
| CRC verification | Not defined by CCSDS 133.0-B-2; governed by the selected external profile. | External / mission-tailored | Non-compliant | `test_v2_crc_mismatch`; one-bit corruption tests |

## ECSS PUS-C traceability matrix

Exact ECSS-E-ST-70-41C clause-level codec traceability is completed by the PUS-C implementation issues. Phase 0 records the selected revision, the mandatory concepts, and the mission-tailoring boundary.

| Requirement area | v2 requirement | Classification | Current status | Required evidence / target test |
|---|---|---|---|---|
| Revision identity | PUS-C is represented explicitly as ECSS-E-ST-70-41C / version 2 | Mandatory | Contract added; codec planned | `test_v2_pus_revision_c_value` |
| Packet direction | TC and TM are modelled separately from revision | Mandatory | Contract added; concrete types planned | `test_v2_tc_tm_type_separation` |
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

The initial mission-tailoring rules, validation constraints, and defaults are defined in [`MISSION_TAILORING.md`](MISSION_TAILORING.md).

A standards-facing encoder shall require an explicit applicable profile before finalization. A standards-facing parser shall receive the applicable profile from the caller or a higher-level routing context.

Code shall not infer:

- the presence of packet error control from trailing bytes;
- a timestamp format from remaining length;
- packet direction from a class name;
- an unknown secondary-header layout as PUS-C;
- identifier width by truncating the supplied value.

## Compliance evidence policy

A matrix row may be changed to **Implemented** only when all of the following exist:

1. an exact normative clause or a documented derived/external requirement;
2. a source-code reference;
3. a focused regression test;
4. an independent expected byte vector or independently calculated value where wire behaviour is involved;
5. a negative test for rejected inputs where validation is involved;
6. no contradictory supported configuration path.

Internal serialization/deserialization round trips are useful regression tests, but they are not independent conformance evidence.

## Issue #44 completion statement

Issue #44, **Create the CCSDS Space Packet compliance matrix**, is complete when this document is merged with:

- exact CCSDS clause references for the supported packet-format requirements;
- Annex A PICS item references and an explicit claim boundary;
- primary-header, Packet Data Length, sequence-control, packet-boundary, parsing, and packet-error-control traceability;
- implementation references and named evidence targets;
- explicit classification of requirements that are derived, external, unsupported, or not applicable.

No row in this Phase 0 baseline is marked **Implemented** without an existing independent test reference. Rows become **Implemented** incrementally through the corresponding implementation issues.

## Phase 0 decision record

Phase 0 is complete when this document, the mission-tailoring specification, and the acceptance-list updates are merged, and the following decisions are treated as change-controlled:

- CCSDS 133.0-B-2 Issue 2 is the packet-layer baseline.
- CCSDSPack v2.0.0 targets Space Packet PDU and applicable library-behaviour conformance, not an unqualified complete protocol-entity PICS claim.
- ECSS-E-ST-70-41C is the primary and only PUS revision targeted by v2.0.0.
- PUS-A is deferred.
- PUS-B is rejected as a standards concept.
- TC and TM are separate directions, not PUS revisions.
- mission-tailored fields are explicit configuration, never parser guesswork.
- unsupported layers, service primitives, transport functions, and services do not appear in v2.0.0 compliance claims.
