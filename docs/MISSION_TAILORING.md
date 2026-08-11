# CCSDSPack v2 PUS tailoring

## Purpose

CCSDSPack separates generic CCSDS Packet policy from optional PUS secondary-header tailoring.

There is no public `MissionProfile` runtime object in v2. A complete `ccsds::Packet` already owns the packet-level settings and installed secondary-header schema required by `ccsds::Manager`, parsing, and validation.

## Ownership model

Generic packet state belongs to `ccsds::Packet`:

- CCSDS primary header;
- `ccsds::PacketDirection`, represented by the primary-header Packet Type;
- `ccsds::PacketErrorControlMode`;
- `ccsds::CRC16Config`;
- data-field capacity and application data;
- the optional secondary-header object.

PUS identity belongs to the concrete secondary-header type:

| Wire layout | C++ type | Intrinsic direction | Canonical selector |
|---|---|---|---|
| PUS-A TC | `ccsds::pus::rev_a::TcHeader` | `Telecommand` | `PUS:revA:TC` |
| PUS-A TM | `ccsds::pus::rev_a::TmHeader` | `Telemetry` | `PUS:revA:TM` |
| PUS-C TC | `ccsds::pus::rev_c::TcHeader` | `Telecommand` | `PUS:revC:TC` |
| PUS-C TM | `ccsds::pus::rev_c::TmHeader` | `Telemetry` | `PUS:revC:TM` |

A PUS class cannot be configured to report another revision or direction. Installing a directional secondary header with `Packet::setSecondaryHeader()` synchronizes the CCSDS primary-header Packet Type.

Custom secondary headers are direction-neutral by default. A custom implementation may override `SecondaryHeaderAbstract::getDirection()` to expose `ccsds::PacketDirection::Telemetry` or `Telecommand` when its own format has an intrinsic direction.

## Packet error control is not PUS tailoring

`ccsds::PacketErrorControlMode::{CRC16,None}` is packet-level policy:

```cpp
ccsds::Packet packet;
packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
```

It works for packets with no secondary header, custom secondary headers, and PUS secondary headers. The optional CRC16 is appended at the end of the CCSDS Packet Data Field; it is not an additional CRC owned by the PUS header.

A Manager template also carries this Packet setting directly. There is no second PUS copy of the PEC choice to synchronize.

## Default PUS usage

The normal PUS path requires no tailoring object.

```cpp
ccsds::Packet packet;

packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});

packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TcHeader>(
    17, 1, 0x1234, 0x09));
```

`rev_c::TcHeader` intrinsically means PUS-C Telecommand and uses the fixed two-octet PUS-C source identifier supported by CCSDSPack. The packet's CCSDS Packet Type is synchronized to Telecommand when the header is installed.

PUS-C TM similarly uses the fixed two-octet destination identifier.

## Optional tailoring types

Tailoring structs contain only actual optional layout choices.

### PUS-A TC

```cpp
ccsds::pus::rev_a::TcTailoring tailoring;
tailoring.sourceIdOctets = 1;
tailoring.secondaryHeaderSpareOctets = 1;
```

Supported PUS-A identifier widths are 0, 1, 2, or 4 octets.

### PUS-A TM

```cpp
ccsds::pus::rev_a::TmTailoring tailoring;
tailoring.destinationIdOctets = 1;
tailoring.packetSubcounterPresent = true;
tailoring.secondaryHeaderSpareOctets = 0;
```

The same tailoring can optionally enable a numeric CUC timestamp.

### PUS-C TC

```cpp
ccsds::pus::rev_c::TcTailoring tailoring;
tailoring.secondaryHeaderSpareOctets = 1;
```

The source identifier remains fixed at two octets and is intentionally not configurable.

### PUS-C TM

```cpp
ccsds::pus::rev_c::TmTailoring tailoring;
tailoring.timestampPresent = true;
tailoring.cuc = {
  ccsds::time::Epoch::Ccsds1958Tai,
  ccsds::time::PFieldMode::Explicit,
  4,
  2
};
tailoring.secondaryHeaderSpareOctets = 0;
```

The destination identifier remains fixed at two octets.

## Numeric CUC time

When TM tailoring enables a timestamp, the header stores numeric coarse/fine counters:

```cpp
ccsds::time::CucTime timestamp{0x01020304, 0xA0B0};
```

The implemented basic CUC support provides:

- 1 through 4 coarse-time octets;
- 0 through 3 fine-time octets;
- the CCSDS epoch at 1958-01-01 TAI or an agency-defined epoch;
- implicit or explicit one-octet P-field policy;
- network-byte-order encoding and exact P-field validation during decoding.

CCSDSPack represents and validates the numeric counter. It does not convert UTC calendar timestamps, maintain leap-second tables, or define an agency epoch.

## Parsing ownership

Parsing never tries to infer PUS revision or optional layout from arbitrary remaining bytes. The caller supplies a secondary-header schema either through the Packet or the typed API.

Preinstalled schema:

```cpp
ccsds::Packet packet;
packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>());
packet.deserialize(wire);
```

Typed default schema:

```cpp
ccsds::Packet packet;
packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire);
```

Typed tailored schema:

```cpp
packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire, tailoring);
```

The parser checks that the CCSDS primary-header Packet Type matches the intrinsic direction of the supplied concrete PUS header.

Raw pointer + size typed parsing is provided through `ccsds::buffer` using the same model.

## Manager ownership

`ccsds::Manager` uses its complete Packet template as the stream contract. The template carries:

- Packet Identification;
- packet-level PEC mode;
- secondary-header presence and concrete type;
- PUS revision/direction through that type;
- optional PUS tailoring through that header instance;
- sequence configuration.

Consequently Manager has no independent PUS profile to keep synchronized.

## Validation ownership

PUS codecs validate their own tailoring and fields while serializing/parsing. `ccsds::Validator` exposes the resulting relationships as named checks including:

- secondary-header direction versus CCSDS Packet Type;
- PUS revision and direction;
- PUS tailoring validity;
- encoded header size;
- reserved/version and spare fields;
- acknowledgement and source/destination identifiers;
- PUS-A TM subcounter state;
- PUS-C TM time-reference status;
- numeric CUC timestamp fit.

Packet-level PEC and template secondary-header tailoring are checked independently. See [VALIDATION.md](VALIDATION.md).

## Configuration ownership

Hosted configuration selects a concrete PUS header using `secondary_header_type`, for example:

```ini
secondary_header_type:string=PUS:revC:TM
```

The selector already defines revision and direction. Configuration therefore does not need a separate mission-profile, PUS-revision, or PUS-direction field. Optional fields such as PUS-A identifier widths or TM CUC layout configure only the applicable header tailoring.

See [CONFIG.md](CONFIG.md) and [`example/config`](../example/config).

`ccsds::Config` is host-only. Bare-metal C++17 applications construct the same concrete PUS headers and optional tailoring structs directly.

## Change control

Adding a tailoring field or support claim requires explicit units and ranges, deterministic serialization/parsing, positive and negative tests, independent wire evidence, and an update to the compliance documentation.
