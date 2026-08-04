# Migrating CCSDSPack v1 to v2

CCSDSPack v2 deliberately breaks the legacy secondary-header API. The old classes were project-specific formats whose names could be mistaken for ECSS Packet Utilisation Standard revisions. They have no compatibility aliases in v2.

## Type replacements

| Removed v1 concept | v2 replacement |
|---|---|
| `PusA` | `PusATcHeader` or `PusATmHeader` with an explicit PUS-A profile |
| `PusB` | No replacement; ECSS-E-70-41B was never issued |
| `PusC` | `PusCTcHeader` or `PusCTmHeader` with an explicit PUS-C profile |
| `PusServices.h/.cpp` | `PusSecondaryHeaders.h/.cpp` |
| Shared prototype registry | Fresh-instance creator registry |
| One mixed custom/PUS registry | `SecondaryHeaderFactory` plus fixed `PusSecondaryHeaderFactory` |

The standards-facing selectors are `PUS:revA:TC`, `PUS:revA:TM`, `PUS:revC:TC`, and `PUS:revC:TM`. The `PUS:` namespace is reserved and cannot be registered by custom headers.

## Construction

Before:

```cpp
packet.setDataFieldHeader(std::make_shared<PusC>(...));
```

After:

```cpp
auto profile = CCSDS::makePusProfile(
  CCSDS::PusRevision::C, CCSDS::PacketDirection::Telemetry);
profile.telemetryTimestampPresent = true;
profile.telemetryTimeCode = CCSDS::TimeCodeFormat::Cuc;
profile.telemetryTimeCodeOctets = 4;

CCSDS::Packet packet;
if (auto result = packet.setMissionProfile(profile); !result) {
  // handle result.error()
}
if (auto result = packet.setDataFieldHeader(
      std::make_shared<CCSDS::PusCTmHeader>(
        profile, 3, 25, 1, 0x0001, 0, std::vector<std::uint8_t>{0, 0, 0, 0}));
    !result) {
  // handle result.error()
}
```

`Packet::setDataFieldHeader(shared_ptr)` now returns `ResultBool`, because profile, namespace, type, and capacity mismatches are checked.

## Parsing

Configure the same profile used by the sender, then provide the canonical selector:

```cpp
CCSDS::Packet decoded;
decoded.setMissionProfile(profile);
auto result = decoded.deserializeBounded(bytes, "PUS:revC:TM");
```

The parser does not infer revision, direction, timestamp size, identifier width, or packet error control from remaining bytes. A generic parsing overload rejects a packet when the active profile requires PUS.

## Wire-format impact

The old classes encoded project-specific fields, including non-standard application-data lengths. The new types encode the revision- and direction-specific secondary-header fields. Existing serialized legacy headers must be regenerated; renaming a selector is not sufficient.

Legacy configuration values `secondary_header_type=PusA`, `PusB`, or `PusC` fail with a migration diagnostic. v2 configuration and CLI profile integration is tracked separately from the C++ codec API.
