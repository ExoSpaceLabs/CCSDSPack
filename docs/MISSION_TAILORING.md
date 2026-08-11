# CCSDSPack v2 mission tailoring

## Purpose

CCSDS packet error control and the ECSS PUS-A/PUS-C secondary-header layouts
contain mission-selected parameters. CCSDSPack records those choices in one
validated `ccsds::MissionProfile`, shared by Packet, Manager, PUS codecs, the
structured Validator, and hosted command-line tools.

## Namespace and type model

Generic Space Packet types live in `ccsds`. Standards-defined PUS types are
grouped by revision and direction:

| Wire profile | C++ type | Canonical selector |
|---|---|---|
| PUS-A TC | `ccsds::pus::rev_a::TcHeader` | `PUS:revA:TC` |
| PUS-A TM | `ccsds::pus::rev_a::TmHeader` | `PUS:revA:TM` |
| PUS-C TC | `ccsds::pus::rev_c::TcHeader` | `PUS:revC:TC` |
| PUS-C TM | `ccsds::pus::rev_c::TmHeader` | `PUS:revC:TM` |

The lowercase namespace is a v2 source-level convention. It does not alter wire
format or runtime performance. The nested PUS namespaces keep revision-specific
codecs separate from the generic packet API.

`ccsds::pus::SecondaryHeaderFactory` owns the fixed standards registry. The
direction-neutral `ccsds::SecondaryHeaderFactory` remains the extension point
for custom secondary headers.

## Generic and PUS profiles

A default-constructed profile represents a generic Space Packet:

```cpp
ccsds::MissionProfile profile;
```

Create a PUS profile with an explicit revision and direction:

```cpp
auto profile = ccsds::pus::makeProfile(
  ccsds::pus::Revision::C,
  ccsds::pus::Direction::Telemetry);
profile.packetErrorControl = ccsds::PacketErrorControlMode::CRC16;
profile.destinationIdOctets = 2;
```

PUS-C requires a two-octet source ID for TC and a two-octet destination ID for
TM. PUS-A identifier widths are mission-tailored to 0, 1, 2, or 4 octets.

## Numeric CUC time

PUS telemetry can carry a numeric CCSDS Unsegmented Time Code (CUC). The profile
defines the wire representation; the header stores numeric coarse and fine
counters:

```cpp
profile.telemetryTimestampPresent = true;
profile.telemetryTimeCode = ccsds::time::Format::Cuc;
profile.telemetryCuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Explicit,
  4,
  2
};

ccsds::time::CucTime timestamp{0x01020304, 0xA0B0};
```

The implemented basic CUC profile supports:

- 1 through 4 coarse-time octets;
- 0 through 3 fine-time octets;
- the CCSDS epoch at 1958-01-01 TAI or an agency-defined epoch;
- implicit or explicit one-octet P-field policy;
- network-byte-order encoding and exact P-field validation during decoding.

CCSDSPack represents the counter and validates its encoding. It does not convert
UTC calendar timestamps, apply leap-second tables, or define an agency epoch.

## Validation ownership

`validateMissionProfile()` validates the configuration of the wire contract
itself. It rejects:

- missing/unsupported PUS revision or direction;
- unsupported identifier widths and incorrect fixed PUS-C widths;
- TC-only fields in TM profiles or TM-only fields in TC profiles;
- timestamp metadata when time is disabled;
- invalid CUC epoch, P-field policy, or coarse/fine widths;
- inconsistent PUS packet-error-control selection.

`ccsds::Packet` and the concrete PUS codecs validate packet/header state during
attachment, parsing, and finalization.

`ccsds::Validator` then exposes those packet/profile relationships as named
checks, including revision, direction, Packet Type, identifiers, reserved/spare
fields, acknowledgement flags, PUS-A TM subcounter policy, PUS-C TM
four-bit time-reference status, CUC timestamp fit, and packet-error-control
consistency. See [VALIDATION.md](VALIDATION.md).

Configured spare-octet **count** belongs to the MissionProfile. Actual encoded
spare octets are required to be zero and are checked by the PUS codec/Validator.

Parsing never guesses revision, direction, identifier widths, time layout, or
packet error control from the remaining bytes.

## Configuration ownership

The same configuration profile is loaded by `ccsds::Packet`, propagated by
`ccsds::Manager`, and used by the hosted encoder, decoder, and validator. See
[CONFIG.md](CONFIG.md) and the committed profiles in
[`example/config`](../example/config).

`ccsds::Config` is host-only. Bare-metal C++17 applications construct the same
MissionProfile directly and use the same packet/PUS/Validator APIs.

CCSDSPack validates and applies mission choices. It does not decide which PUS
services, identifiers, packet-error control, epoch, or time resolution a mission
should use.

## Change control

Adding a profile field or support claim requires explicit units and ranges,
deterministic serialization/parsing, positive and negative tests, independent
wire evidence, and an update to the compliance documentation.
