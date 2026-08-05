# CCSDSPack v2 mission tailoring

## Purpose

CCSDS packet error control and the ECSS PUS-A/PUS-C secondary-header layouts
contain mission-selected parameters. CCSDSPack records those choices in one
validated `ccsds::MissionProfile`, which is shared by Packet, Manager, and the
command-line tools.

## Namespace and type model

Generic Space Packet types live in `ccsds`. Standards-defined PUS types are
grouped by revision and direction:

| Wire profile | C++ type | Canonical selector |
|---|---|---|
| PUS-A TC | `ccsds::pus::rev_a::TcHeader` | `PUS:revA:TC` |
| PUS-A TM | `ccsds::pus::rev_a::TmHeader` | `PUS:revA:TM` |
| PUS-C TC | `ccsds::pus::rev_c::TcHeader` | `PUS:revC:TC` |
| PUS-C TM | `ccsds::pus::rev_c::TmHeader` | `PUS:revC:TM` |

The lowercase namespace is a v2 source-level convention. It does not alter the
wire format or runtime performance. The nested PUS namespaces keep
standards-specific codecs out of the generic packet API and prevent a revision
from being confused with packet direction.

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
defines the wire representation; the header stores the numeric coarse and fine
counters:

```cpp
profile.telemetryTimestampPresent = true;
profile.telemetryTimeCode = ccsds::time::Format::Cuc;
profile.telemetryCuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Explicit,
  4, // coarse octets
  2  // fine octets
};

ccsds::time::CucTime timestamp{
  0x01020304, // integral seconds from the selected epoch
  0xA0B0      // binary fraction scaled by 2^(8 * fineOctets)
};
```

The implemented basic CUC profile supports:

- 1 through 4 coarse-time octets;
- 0 through 3 fine-time octets;
- the CCSDS epoch at 1958-01-01 TAI or an agency-defined epoch;
- an implicit P-field supplied by the mission profile or an explicit one-octet
  P-field carried on the wire;
- network-byte-order encoding and exact P-field validation during decoding.

CCSDSPack represents the counter and validates its encoding. It does not convert
UTC calendar timestamps, apply leap-second tables, or define an agency epoch.
Those responsibilities belong to the mission time-correlation layer and its
interface control document.

## Validation rules

A profile is rejected when it is ambiguous or permits a different wire layout.
The checks include:

- explicit supported PUS revision and direction;
- TC/TM consistency with the primary-header Packet Type;
- supported identifier widths and PUS-C fixed widths;
- no TC-only fields in TM profiles or TM-only fields in TC profiles;
- no timestamp metadata when time is disabled;
- valid CUC epoch, P-field policy, and coarse/fine widths;
- counter values fitting their configured widths;
- consistent packet-error-control policy;
- zero-valued configured spare octets.

Parsing never guesses revision, direction, identifier widths, time layout, or
packet error control from the remaining bytes.

## Configuration ownership

The same configuration profile is loaded by `ccsds::Packet`, propagated by
`ccsds::Manager`, and used by the encoder, decoder, and validator. See
[CONFIG.md](CONFIG.md) for the complete schema and the committed profiles in
[`example/config`](../example/config).

CCSDSPack validates and applies mission choices. It does not decide which PUS
services, identifiers, packet-error control, epoch, or time resolution a mission
should use.

## Change control

Adding a profile field or support claim requires explicit units and ranges,
deterministic serialization and parsing, positive and negative tests,
independent wire vectors, and an update to the compliance matrix.
