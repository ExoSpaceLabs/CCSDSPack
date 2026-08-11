# CCSDSPack v2 PUS tailoring

## Purpose

CCSDSPack separates generic Space Packet policy from optional PUS secondary-header layout choices. This keeps packet-level concerns such as Packet Type and CRC policy independent from revision-specific PUS fields.

## Ownership model

A complete `ccsds::Packet` owns:

- the CCSDS primary header;
- packet direction through the primary-header Packet Type;
- `ccsds::PacketErrorControlMode` and CRC configuration;
- data-field capacity and application data;
- the optional secondary-header object.

PUS identity belongs to the concrete secondary-header type:

| Wire layout | C++ type | Direction | Selector |
|---|---|---|---|
| PUS-A TC | `ccsds::pus::rev_a::TcHeader` | Telecommand | `PUS:revA:TC` |
| PUS-A TM | `ccsds::pus::rev_a::TmHeader` | Telemetry | `PUS:revA:TM` |
| PUS-C TC | `ccsds::pus::rev_c::TcHeader` | Telecommand | `PUS:revC:TC` |
| PUS-C TM | `ccsds::pus::rev_c::TmHeader` | Telemetry | `PUS:revC:TM` |

Installing a directional secondary header synchronizes the CCSDS Packet Type and Secondary Header Flag. Custom secondary headers are direction-neutral by default and may override `getDirection()` when their own format has an intrinsic direction.

## Packet error control

`PacketErrorControlMode::{CRC16,None}` is Packet-level policy and applies uniformly to packets with no secondary header, a custom header, or a PUS header. When CRC16 is selected, the trailer is encoded at the end of the CCSDS Packet Data Field.

This gives packet error control one source of truth and keeps PUS tailoring limited to PUS wire-layout decisions.

## Default PUS construction

PUS headers provide standards-compatible default tailoring:

```cpp
ccsds::Packet packet;
packet.setPrimaryHeader(ccsds::PrimaryHeader{
  0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});

packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TcHeader>(
    17, 1, 0x1234, 0x09));
```

`rev_c::TcHeader` identifies PUS-C Telecommand directly. PUS-C TC source ID and PUS-C TM destination ID use the fixed two-octet widths of the supported layout.

## Direction-specific tailoring

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

PUS-A TM tailoring can also enable a numeric CUC timestamp.

### PUS-C TC

```cpp
ccsds::pus::rev_c::TcTailoring tailoring;
tailoring.secondaryHeaderSpareOctets = 1;
```

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
```

## Numeric CUC time

TM tailoring can store a `ccsds::time::CucTime` numeric coarse/fine counter. The supported subset provides:

- 1–4 coarse-time octets;
- 0–3 fine-time octets;
- CCSDS 1958 TAI or agency-defined epoch metadata;
- implicit or explicit basic one-octet P-field;
- network-byte-order encoding and exact P-field validation.

The codec does not perform UTC/calendar conversion, leap-second processing, agency-epoch definition, or time correlation.

## Parsing

Parsing uses an explicit secondary-header schema. A caller may preinstall a header:

```cpp
ccsds::Packet packet;
packet.setSecondaryHeader(
  std::make_shared<ccsds::pus::rev_c::TmHeader>());
packet.deserialize(wire);
```

or use typed parsing:

```cpp
packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire);
```

Tailoring constructor arguments can be supplied for layouts that enable optional fields. Typed raw pointer-plus-size parsing is available through `ccsds::buffer` with the same model.

## Manager and validation

`ccsds::Manager` uses its complete Packet template as the stream contract. The same object therefore carries Packet Identification, packet-level PEC, concrete PUS identity, and optional tailoring.

`ccsds::Validator` checks secondary-header direction, concrete PUS revision/direction, tailoring validity, encoded header size, reserved/spare fields, identifiers, counters, and active CUC timestamp state. Packet-level PEC is checked independently.

See [VALIDATION.md](VALIDATION.md).

## Configuration

Hosted configuration selects a concrete PUS identity through `secondary_header_type`, for example:

```ini
secondary_header_type:string=PUS:revC:TM
```

Optional configuration keys then apply only to the selected header's supported tailoring and packet fields. See [CONFIG.md](CONFIG.md).
