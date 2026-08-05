# CCSDSPack v2.0.0 compliance baseline

## Scope

CCSDSPack v2 inherits the validated generic Space Packet PDU behaviour from v1.2 and adds revision- and direction-specific PUS secondary-header codecs.

| Area | Baseline | Implemented scope |
|---|---|---|
| Space Packet | CCSDS 133.0-B-2 Issue 2, including Editorial Change 2 | Construction, serialization, bounded parsing, inspection, validation, segmentation, and packet-error-control profile |
| PUS-A | ECSS-E-70-41A, 30 January 2003 | TC and TM secondary-header layouts |
| PUS-C | ECSS-E-ST-70-41C, 15 April 2016 | TC and TM secondary-header layouts |
| CUC time | CCSDS 301.0-B-4 | Basic numeric CUC with selected epoch, P-field, and coarse/fine widths |

This is not a claim to implement every PUS service, UTC/calendar time
conversion, leap-second processing, mission time correlation, transfer frames,
COP-1, CFDP, a transport binding, or a complete protocol entity.

## PUS public model

The standards-facing types are:

- `ccsds::pus::rev_a::TcHeader` and `ccsds::pus::rev_a::TmHeader`;
- `ccsds::pus::rev_c::TcHeader` and `ccsds::pus::rev_c::TmHeader`.

PUS revision and packet direction are independent strong types. Canonical selectors are `PUS:revA:TC`, `PUS:revA:TM`, `PUS:revC:TC`, and `PUS:revC:TM`.

There is no PUS-B revision. The former project-specific `PusA`, `PusB`, and `PusC` classes and `PusServices` translation unit are absent from the v2 runtime and public API.

## Factory boundary

`ccsds::SecondaryHeaderFactory` is the direction-neutral extension point for
custom and opaque headers. It is string-keyed, returns a fresh instance per
creation, rejects duplicate keys, and reserves the `PUS:` namespace.

`ccsds::pus::SecondaryHeaderFactory` is fixed and non-extensible. It constructs
only the four supported standards codecs from a validated mission profile. User
code cannot replace a standards selector.

## Mission-profile rules

A default `MissionProfile` is generic CCSDS and contains no implied PUS selection. A PUS profile explicitly selects:

- PUS-A or PUS-C;
- TC or TM;
- applicable source/destination identifier widths;
- packet error control;
- TM timestamp presence and basic CUC epoch/P-field/coarse/fine layout;
- PUS-A TM packet-subcounter presence;
- octet-aligned zero spare fields.

PUS-C uses the standard two-octet source ID for TC and destination ID for TM. PUS-A supports the mission-defined optional identifier fields exposed by the profile. Invalid or direction-inapplicable combinations return errors without normalization.

The time codec stores numeric coarse/fine CUC counters, constructs or verifies
the supported basic one-octet P-field, and validates counter widths. The CCSDS
1958 TAI epoch and an agency-defined epoch are selectable. Calendar conversion,
leap-second handling, agency-epoch definition, and time correlation are outside
this implementation.

## PUS-A layout coverage

TC coverage:

- zero CCSDS-secondary-header indicator bit;
- PUS version 1;
- four acknowledgement bits;
- service type and subtype;
- mission-selected optional source ID;
- zero spare bytes.

TM coverage:

- reserved zero bits and PUS version 1;
- service type and subtype;
- optional packet subcounter;
- mission-selected optional destination ID;
- optional timestamp;
- zero spare bytes.

## PUS-C layout coverage

TC coverage:

- PUS version 2;
- four acknowledgement bits;
- service type and subtype;
- two-octet source ID;
- zero spare bytes.

TM coverage:

- PUS version 2;
- four-bit time-reference status;
- service type and subtype;
- two-octet message-type counter;
- two-octet destination ID;
- optional timestamp;
- zero spare bytes.

## Packet integration

`ccsds::Packet::setMissionProfile` validates and installs the profile and
synchronizes packet-error-control mode. PUS attachment and parsing reject:

- revision/direction/profile mismatch;
- TC/TM mismatch with the CCSDS primary-header Packet Type;
- packet-error-control mismatch;
- custom headers under a PUS profile;
- generic parsing while a PUS profile requires an explicit selector;
- malformed version/reserved/spare fields;
- identifier and timestamp sizes inconsistent with the profile;
- invalid CUC epoch, P-field, coarse/fine widths, or overflowing counters.

## Evidence

The current native suite contains 106 passing tests. Evidence includes fixed
TC/TM byte vectors for both revisions, explicit and implicit P-field CUC vectors,
fresh-instance factory checks, all four configuration selectors, Manager PUS
parsing, wrong-direction/profile/error-control failures, version and
reserved-bit rejection, timestamp overflow, and spare-byte validation. CLI
integration round-trips generic, PUS-A TC/TM, and PUS-C TC/TM configurations and
rejects a corrupted PUS version.

The library also compiles in MCU mode with `-fno-exceptions -fno-rtti`. Generic v1.2 independent CCSDS vectors and regression tests remain passing.

CI, installed-package, Doxygen, cross-platform, sanitizer, fuzz, arm64, and hardware release gates remain authoritative at integration/release time and are not implied by a local compile.
