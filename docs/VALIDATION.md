# Structured packet validation

CCSDSPack v2 exposes packet validation through `ccsds::Validator` and the fixed-capacity `ccsds::ValidationReport`.

The validator is part of the C++17 library and is compiled in both hosted and `CCSDS_MCU` builds. It does not require exceptions or RTTI. The report itself stores checks in a fixed `std::array` and performs no dynamic allocation.

## Basic use

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);

if (!report.valid()) {
  for (const auto &check : report) {
    if (!check.passed) {
      handleValidationFailure(check.code,
                              ccsds::validationCodeName(check.code));
    }
  }
}
```

`validate()` does not modify the `Packet` or its secondary header. A Validator retains sequence-stream state between calls when sequence validation is enabled.

## Ownership model

Validation follows the v2 object model directly:

- packet-level PEC/CRC belongs to `ccsds::Packet`;
- packet direction is represented by the CCSDS primary-header Packet Type;
- a directional secondary header reports `ccsds::PacketDirection`;
- PUS revision and direction are intrinsic to the concrete PUS header class;
- optional PUS layout choices are stored in that header's tailoring;
- a `ccsds::Manager` template is a complete Packet contract rather than a separate profile.

There is no parallel `MissionProfile` to validate or synchronize.

## Structured checks

Generic Space Packet checks include:

- `PrimaryHeader`;
- `PacketVersion`;
- `PacketDataLength`;
- `Crc16` when CRC16 packet error control is enabled;
- `SecondaryHeaderPresence`;
- `SecondaryHeaderDirection` for a directional header;
- `SequenceFlags`;
- `SequenceCount` when sequence-count validation is enabled;
- `PacketIdentifier` and `SegmentationClass` when template comparison is enabled;
- `TemplatePacketErrorControl`;
- `TemplateSecondaryHeader`.

Standards PUS headers additionally expose:

- `PusHeader`;
- `PusRevision`;
- `PusDirection`;
- `PusPacketType`;
- `PusTailoring`;
- `PusSecondaryHeaderSize`;
- `PusReservedBits`;
- `PusSpareFields`;
- `PusAcknowledgement` and `PusSourceId` for telecommands;
- `PusDestinationId` for telemetry;
- `PusPacketSubcounter` for PUS-A telemetry;
- `PusTimeReferenceStatus` for PUS-C telemetry;
- `PusTimestamp` when checking the active CUC timestamp state.

Only checks that are relevant and actually performed are stored. A packet using `PacketErrorControlMode::None`, for example, does not create a `Crc16` entry.

## Querying a report

```cpp
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::PacketDataLength)) {
  handleLengthMismatch();
}

if (report.failed(ccsds::ValidationCode::SecondaryHeaderDirection)) {
  handleDirectionMismatch();
}

if (report.failed(ccsds::ValidationCode::PusTailoring)) {
  handleInvalidPusLayout();
}
```

`ValidationReport::Capacity` is fixed at 32 checks. The v2 validation set remains bounded below that capacity.

## Stateful sequence validation

`Validator` represents one sequence-validation stream. With packet-coherence and sequence-count validation enabled, it checks:

- legal segmented/unsegmented transitions;
- Packet Sequence Count continuity;
- modulo-16384 rollover.

Sequence state advances only after the enabled validation checks for the packet pass. Call `clear()` before reusing the Validator for an unrelated stream.

## Template validation

A complete Packet template can be installed with `setTemplatePacket()`:

```cpp
ccsds::Validator validator;
validator.setTemplatePacket(packetTemplate);
validator.configure(true, true, true);
```

Template validation compares:

- Packet Identification;
- segmented versus unsegmented class;
- packet-level PEC mode;
- secondary-header presence, concrete type/direction, and PUS tailoring when applicable.

Packet Sequence Count and Packet Data Length are not treated as fixed Packet Identification fields.

## Parser validation versus object validation

Malformed wire bytes can fail before a usable Packet object is produced. `deserializeBounded()` remains responsible for bounded packet size, primary-header decoding, CRC verification, secondary-header size, and secondary-header decoding.

For PUS parsing the caller supplies the concrete schema either by preinstalling a header or using a typed parser:

```cpp
ccsds::Packet packet;
const auto parsed =
  packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire);
```

If the concrete header direction contradicts the CCSDS Packet Type, parsing fails before the object is committed.

`Validator` then operates on the resulting Packet and exposes named checks for its stored state. The command-line `ccsds_validator` uses the same library paths rather than maintaining separate protocol-validation logic.

## Bare-metal build

```bash
cmake -S . -B build-mcu \
  -DCCSDSPACK_BUILD_MCU=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DCCSDSPACK_MCU_FLAGS="-fno-exceptions -fno-rtti"
```

The project language standard remains C++17. Host-only `ccsds::Config` and command-line tools are excluded from `CCSDS_MCU`, while `Packet`, PUS tailoring/types, `Validator`, `ValidationCode`, and `ValidationReport` remain available to the static-library consumer.
