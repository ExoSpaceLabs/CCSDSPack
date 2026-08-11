# Structured packet validation

CCSDSPack exposes packet validation through `ccsds::Validator` and the fixed-capacity `ccsds::ValidationReport`.

The Validator is part of both hosted and `CCSDS_MCU` C++17 builds. It does not require RTTI or exceptions. The report stores checks in `std::array` and performs no dynamic allocation itself.

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

Validation is read-only with respect to the Packet and its secondary header. A Validator retains sequence-stream state between calls when sequence validation is enabled.

## Validation model

Validation follows the object model directly:

- Packet-level PEC/CRC belongs to `ccsds::Packet`;
- packet direction is represented by the CCSDS primary-header Packet Type;
- directional secondary headers report `ccsds::PacketDirection`;
- PUS revision/direction are intrinsic to the concrete PUS header class;
- optional PUS layout choices are stored in that header's tailoring;
- a Manager Packet template is the complete stream contract.

This provides one validation source for the same state used by serialization and parsing.

## Named checks

Generic checks include:

- `PrimaryHeader`;
- `PacketVersion`;
- `PacketDataLength`;
- `Crc16` when enabled;
- `SecondaryHeaderPresence`;
- `SecondaryHeaderDirection`;
- `SequenceFlags`;
- `SequenceCount`;
- `PacketIdentifier`;
- `SegmentationClass`;
- `TemplatePacketErrorControl`;
- `TemplateSecondaryHeader`.

PUS headers additionally expose applicable checks for:

- `PusHeader`;
- `PusRevision`;
- `PusDirection`;
- `PusPacketType`;
- `PusTailoring`;
- `PusSecondaryHeaderSize`;
- `PusReservedBits`;
- `PusSpareFields`;
- `PusAcknowledgement`;
- `PusSourceId`;
- `PusDestinationId`;
- `PusPacketSubcounter`;
- `PusTimeReferenceStatus`;
- `PusTimestamp`.

Only checks that are actually performed are stored. For example, a Packet using `PacketErrorControlMode::None` does not produce a `Crc16` entry.

`ValidationReport::Capacity` is 32 checks.

## Stateful sequence validation

A Validator represents one sequence-validation stream. When enabled it checks legal segmented/unsegmented transitions, Packet Sequence Count continuity, and modulo-16384 rollover. Sequence state advances only after the enabled checks for a packet pass.

Call `clear()` before using a Validator for an unrelated stream.

## Template validation

A complete Packet template can be installed with `setTemplatePacket()`:

```cpp
ccsds::Validator validator;
validator.setTemplatePacket(packetTemplate);
validator.configure(true, true, true);
```

Template validation compares Packet Identification, segmentation class, packet-level PEC, secondary-header presence, concrete secondary-header type/direction, and PUS tailoring where applicable. Packet Sequence Count and Packet Data Length remain stream-varying fields.

## Parser validation and object validation

Wire-level failures can occur before a complete Packet object exists. `deserializeBounded()` is responsible for packet boundary checks, primary-header parsing, CRC verification, and secondary-header decoding. It commits state only after parsing succeeds.

`Validator` then checks the resulting object and its relationships. The hosted `ccsds_validator` command uses the same Packet parser and Validator implementation rather than a parallel protocol checker.

## Bare-metal build

```bash
cmake -S . -B build-mcu \
  -DCCSDSPACK_BUILD_MCU=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DCCSDSPACK_MCU_FLAGS="-fno-exceptions -fno-rtti"
```

Host-only `ccsds::Config` and command-line tools are excluded from `CCSDS_MCU`; Packet, Manager, PUS codecs/tailoring, CUC time, Result/Error, raw-buffer adapters, `Validator`, `ValidationCode`, and `ValidationReport` remain available.
