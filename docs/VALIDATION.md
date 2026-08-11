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

`validate()` does not modify the `Packet`, its mission profile, or its secondary header. A Validator does retain sequence-stream state between calls when sequence validation is enabled.

## Structured checks

`ValidationCode` names the performed checks. Callers do not depend on positional report indices.

Generic Space Packet checks include:

- `PrimaryHeader`;
- `PacketVersion`;
- `PacketDataLength`;
- `Crc16` when CRC16 packet error control is enabled;
- `SecondaryHeaderPresence`;
- `SequenceFlags`;
- `SequenceCount` when sequence-count validation is enabled;
- `PacketIdentifier` and `SegmentationClass` when template comparison is enabled;
- `MissionProfile` and `TemplateMissionProfile`.

PUS profiles additionally expose checks for:

- `PacketErrorControlProfile`;
- `PusHeader`;
- `PusRevision`;
- `PusDirection`;
- `PusPacketType`;
- `PusProfile`;
- `PusSecondaryHeaderSize`;
- `PusReservedBits`;
- `PusSpareFields`;
- `PusAcknowledgement` and `PusSourceId` for telecommands;
- `PusDestinationId` for telemetry;
- `PusPacketSubcounter` for PUS-A telemetry;
- `PusTimeReferenceStatus` for PUS-C telemetry;
- `PusTimestamp` for the configured CUC timestamp policy.

Only checks that are relevant and actually performed are stored in a report. For example, a CRC-free profile does not create a `Crc16` entry.

## Querying a report

```cpp
const auto report = validator.validate(packet);

if (report.failed(ccsds::ValidationCode::PacketDataLength)) {
  // handle a length mismatch
}

if (report.failed(ccsds::ValidationCode::PusDirection)) {
  // handle a revision/direction/profile mismatch
}

if (report.contains(ccsds::ValidationCode::Crc16)
    && report.failed(ccsds::ValidationCode::Crc16)) {
  // CRC16 was enabled and failed
}
```

`ValidationReport::Capacity` is fixed at 32 checks. The v2 validation set is bounded below that capacity.

## Stateful sequence validation

`Validator` represents one sequence-validation stream. With packet-coherence and sequence-count validation enabled, it checks:

- legal segmented/unsegmented transitions;
- Packet Sequence Count continuity;
- modulo-16384 rollover.

Sequence state advances only after the enabled validation checks for the packet pass. Call `clear()` before reusing the Validator for an unrelated stream.

## Template validation

A packet template can be installed with `setTemplatePacket()` and enabled with `configure()`:

```cpp
ccsds::Validator validator;
validator.setTemplatePacket(packetTemplate);
validator.configure(true, true, true);
```

Template validation compares:

- the complete Packet Identification value;
- segmented versus unsegmented class;
- the complete `MissionProfile` wire tailoring.

Sequence Flags, Packet Sequence Count, and Packet Data Length are not treated as fixed Packet Identification fields.

## Parser validation versus object validation

Malformed wire bytes can fail before a `Packet` object is produced. `deserializeBounded()` remains responsible for bounded parsing, primary-header decoding, declared packet size, CRC verification, and secondary-header decoding.

`Validator` operates on an existing `Packet` and provides structured named checks for the packet state and the selected profile. The command-line `ccsds_validator` uses these library paths rather than maintaining a separate protocol-validation implementation.

## Bare-metal profile

The validation API is intentionally compatible with the v2 MCU build:

```bash
cmake -S . -B build-mcu \
  -DCCSDSPACK_BUILD_MCU=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \
  -DCCSDSPACK_MCU_FLAGS="-fno-exceptions -fno-rtti"
```

The project language standard remains C++17. Host-only `ccsds::Config` and command-line tools are excluded from `CCSDS_MCU`, while `ccsds::Validator`, `ValidationCode`, and `ValidationReport` remain part of the static-library API.
