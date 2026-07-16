# CCSDSPack v1.2 CCSDS 133.0-B-2 profile

CCSDSPack v1.2 implements a **CCSDS 133.0-B-2 Issue 2 Space Packet PDU profile**, including the published Editorial Change 2 from September 2024.

Editorial Change 2 changes document presentation only. It does not change the Space Packet primary header, Packet Data Length, APID, segmentation, or sequence-count semantics.

## Conformity scope

The conformity claim applies to construction, serialization, bounded parsing, and validation of the CCSDS Space Packet Protocol Data Unit:

- six-octet Packet Primary Header;
- Packet Version Number encoded as zero;
- Packet Type, Secondary Header Flag, and 11-bit APID;
- Sequence Flags and 14-bit Packet Sequence Count;
- Packet Data Length encoded as the number of Packet Data Field octets minus one;
- optional secondary-header and mission data carried inside the Packet Data Field;
- exact packet-boundary handling for concatenated inputs.

CCSDSPack does **not** claim to implement the complete abstract Packet Service, Octet String Service, protocol procedures, managed-parameter interface, or normative PICS from CCSDS 133.0-B-2.

## Packet Version Number

CCSDS Space Packets use encoded Packet Version Number `000`. Header assignment, packed-header parsing, configuration loading, and serialization reject any non-zero value.

## Idle Packets

APID `0x7FF` identifies an Idle Packet. CCSDSPack enforces the standard structural restriction that an Idle Packet cannot carry a Packet Secondary Header and therefore requires the Secondary Header Flag to be zero.

The contents of the Idle Data are mission-defined. CCSDSPack validates the packet structure but does not prescribe a mission-specific idle fill pattern.

## Packet Sequence Count profile

CCSDSPack uses Packet Sequence Count semantics for telemetry and telecommand packets. Telecommand Packet Name semantics are not implemented.

`CCSDS::Manager` maintains one modulo-16384 sequence-count stream for its bound Packet Identification. Automatic mode advances once per generated packet. Manual mode is an expert override and can produce a sequence that does not satisfy the continuity requirements of a managed data path.

A mission shall use one sequence-count authority for each APID within a managed data path.

## Packet size

The standard permits a Packet Data Field from one through 65,536 octets, resulting in a total serialized packet size from seven through 65,542 octets.

Use:

```cpp
std::size_t size = packet.getSerializedSize();
```

for the complete size. The legacy `getFullPacketLength()` API returns `std::uint16_t` and cannot represent every valid total packet size; it remains available for v1 source compatibility.

## CCSDSPack CRC16 mission profile

CCSDS 133.0-B-2 defines a Space Packet as a Packet Primary Header followed by a Packet Data Field. It does not define a separate standardized Packet Error Control field.

When `PacketErrorControlMode::CRC16` is selected, CCSDSPack reserves the final two octets of the **Packet Data Field** for a CRC-16/CCITT-FALSE mission-profile trailer. Those two octets are included in Packet Data Length. CRC coverage starts at the first Packet Primary Header octet and ends immediately before the two trailer octets.

`PacketErrorControlMode::None` disables this CCSDSPack trailer. CRC16 remains the default for compatibility with existing v1 construction and configuration paths.

## Secondary headers

Custom registered headers and the bundled `PusA`, `PusB`, and `PusC` types occupy mission-defined ancillary-data bytes inside the Packet Data Field.

The bundled `PusA`, `PusB`, and `PusC` layouts are legacy, project-specific formats. They are not claimed to implement an ECSS Packet Utilisation Standard revision. Standards-oriented ECSS PUS support remains v2 scope.

## Evidence

The Linux and Windows test matrix includes dedicated coverage for:

- rejection of non-zero Packet Version Numbers through setters, structured assignment, configuration, and parsing;
- Idle APID and Secondary Header Flag combinations in both setter orders;
- valid Idle Packet serialization without a secondary header;
- the maximum 65,542-octet serialized size through `getSerializedSize()`;
- telecommand Packet Sequence Count preservation;
- independent Packet Data Length, CRC16, APID, rollover, and concatenated packet vectors.
