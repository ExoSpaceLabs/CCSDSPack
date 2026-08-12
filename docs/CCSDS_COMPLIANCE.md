# CCSDSPack v2.0.0 PUS and CUC compliance baseline

## Scope

| Area | Baseline | Implemented scope |
|---|---|---|
| Space Packet | CCSDS 133.0-B-2 Issue 2, including Editorial Change 2 | Construction, serialization, bounded parsing, inspection, structured validation, segmentation, stream management, packet-level error-control selection |
| PUS-A | ECSS-E-70-41A, 30 January 2003 | Supported TC and TM secondary-header layouts |
| PUS-C | ECSS-E-ST-70-41C, 15 April 2016 | Supported TC and TM secondary-header layouts |
| CUC time | CCSDS 301.0-B-4 | Basic numeric CUC with selected epoch metadata, P-field mode, and coarse/fine widths |

Complete PUS services, calendar/UTC conversion, leap-second processing, mission time correlation, transfer frames, COP-1, CFDP, transport bindings, and a complete protocol entity are outside scope.

## Public PUS model

The standards-facing secondary-header types are:

- `ccsds::pus::rev_a::TcHeader` and `ccsds::pus::rev_a::TmHeader`;
- `ccsds::pus::rev_c::TcHeader` and `ccsds::pus::rev_c::TmHeader`.

Revision and direction are intrinsic to the concrete class. `ccsds::PacketDirection` is used where generic code needs to query a directional secondary header. Canonical selectors are `PUS:revA:TC`, `PUS:revA:TM`, `PUS:revC:TC`, and `PUS:revC:TM`.

`ccsds::SecondaryHeaderFactory` remains the extensible direction-neutral custom-header factory and reserves the `PUS:` selector namespace. `ccsds::pus::SecondaryHeaderFactory` provides the fixed standards selectors.

## Tailoring model

Generic Packet policy belongs to `ccsds::Packet`, including Packet Type, packet-level PEC mode, CRC parameters, data-field capacity, and the installed secondary-header object.

PUS tailoring contains only optional secondary-header layout choices:

- PUS-A TC: optional source-ID width and spare octets;
- PUS-A TM: optional destination-ID width, packet subcounter, CUC timestamp, and spare octets;
- PUS-C TC: spare octets, with fixed two-octet source ID;
- PUS-C TM: optional CUC timestamp and spare octets, with fixed two-octet destination ID.

Invalid widths, overflowing identifiers/counters, invalid CUC configuration, non-zero spare bytes, and direction-inapplicable states are rejected.

## PUS-A coverage

Telecommand coverage includes PUS version 1, acknowledgement bits, service type/subtype, optional source ID, and spare fields.

Telemetry coverage includes PUS version 1, service type/subtype, optional packet subcounter, optional destination ID, optional numeric CUC timestamp, and spare fields.

## PUS-C coverage

Telecommand coverage includes PUS version 2, acknowledgement bits, service type/subtype, fixed two-octet source ID, and spare fields.

Telemetry coverage includes PUS version 2, four-bit time-reference status, service type/subtype, two-octet message-type counter, fixed two-octet destination ID, optional numeric CUC timestamp, and spare fields.

### Independent TC acknowledgement evidence

ECSS-E-ST-70-41C clause 7.4.4.1 defines the TC secondary-header first octet as four PUS-version bits followed by four independently selectable acknowledgement bits. PUS-C requires version 2. The acknowledgement bits request successful acceptance, start, progress, and completion reports respectively.

`test/src/testGroupEvidence.cpp` contains literal independent expected bytes for all 16 valid acknowledgement combinations. With service type `0x11`, subtype `0x01`, and source ID `0x1234`, the vectors run from `20 11 01 12 34` for ACK `0x0` through `2F 11 01 12 34` for ACK `0xF`. Each literal vector is checked against serialization and then independently decoded.

The source derivation and complete table are maintained in `docs/PUS_C_EVIDENCE.md`.

## Numeric CUC subset

The CUC codec supports:

- CCSDS 1958 TAI or agency-defined epoch metadata;
- implicit or explicit basic one-octet P-field;
- 1 to 4 coarse octets;
- 0 to 3 fine octets;
- exact P-field verification and counter-width validation.

The codec stores numeric coarse/fine counters. Calendar conversion, leap-second handling, agency-epoch definition, and time correlation remain outside scope.

## Packet integration

Installing a directional PUS header synchronizes the CCSDS secondary-header flag and Packet Type. Serialization and parsing enforce the concrete header direction, field sizes, reserved bits, spare bytes, identifiers, optional fields, and active CUC configuration.

Packet error control is validated independently of PUS tailoring. `PacketErrorControlMode::CRC16` and `None` apply equally to generic, custom-secondary-header, and PUS packets.

## Structured validation

`ccsds::Validator` uses named `ValidationCode` entries stored in a fixed-capacity `ValidationReport`. Applicable checks include primary-header/version, Packet Data Length, CRC16, secondary-header presence/direction, sequence state, Packet-template comparison, concrete PUS identity/tailoring, header size, reserved/spare fields, TC acknowledgement/source ID, TM destination ID, PUS-A TM packet subcounter, PUS-C TM time-reference status, and CUC timestamp fit.

Validation is read-only with respect to the Packet and secondary header. The Validator owns only its sequence-stream validation state.

All 26 public `ValidationCode` entries are traced in `docs/VALIDATION_EVIDENCE.md`. Direct fixtures added for release hardening cover previously implicit primary-header, Packet Version, secondary-header presence, PUS revision/direction/tailoring, TM destination ID, and PUS-C time-reference status failures. `PusHeader` is documented as a positive classification entry rather than given a contrived impossible failure fixture.

## Robustness

The release CI runs the complete native suite under dedicated Clang AddressSanitizer and UndefinedBehaviorSanitizer jobs. Four bounded libFuzzer targets run with ASan+UBSan for declared-size/primary-header inspection, generic Packet parsing, typed PUS-A/PUS-C parsing, and CUC decode/encode behavior.

The fuzz smoke gate limits generated-input count, input length, per-input timeout, and RSS. This provides repeatable crash, over-read, undefined-behavior, timeout, and bounded-resource evidence. It does not imply that the vector-backed v2 object model is zero-copy or globally heap-free. Detailed settings and assertions are in `docs/ROBUSTNESS.md`.

## Evidence

The current release candidate has **132/132 native regression/conformance tests** covering generic packet behavior, all four PUS concrete identities, tailoring, CUC vectors, configuration selectors, Manager parsing, raw-buffer interfaces, structured validation, the complete PUS-C acknowledgement matrix, and the final named negative-validation matrix.

CLI integration covers generic and representative PUS streams and malformed PUS input. Linux/Windows CI, Doxygen, installed-package examples/consumer, Ubuntu 22.04 package/cross-build generation, and the Cortex-M compile/link probe provide integration evidence. Dedicated ASan, UBSan, and bounded four-target libFuzzer CI provide automated robustness evidence.

Fresh native arm64 execution and fresh physical STM32 execution remain release-level acceptance gates.
