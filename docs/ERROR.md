<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Error and Result handling

[Documentation index](README.md) | [Structured validation](VALIDATION.md) | [Raw buffers](RAW_BUFFERS.md) | [API reference](https://exospacelabs.github.io/CCSDSPack/html/)

CCSDSPack reports checked operation failures through `ccsds::Result<T>` and
`ccsds::Error`, defined in `inc/CCSDSResult.h`. Public packet and Manager
operations do not use exceptions as their normal error channel.

## Basic Result pattern

```cpp
const auto result = packet.deserializeBounded(bytes);
if (!result) {
  log(result.error().code(), result.error().message());
  return result.error().code();
}

const std::size_t consumed = result.value();
```

A `Result<T>` contains either a success value or an `Error`. Check the result
before calling `value()` or `error()`.

## Error categories

| Name | Value | Meaning |
|---|---:|---|
| `NONE` | 0 | No error. |
| `UNKNOWN_ERROR` | 1 | Unclassified failure. |
| `NO_DATA` | 2 | Required data or stored packets are absent. |
| `INVALID_DATA` | 3 | Generic malformed data or invalid boundary. |
| `INVALID_HEADER_DATA` | 4 | Invalid primary-header field or bytes. |
| `INVALID_SECONDARY_HEADER_DATA` | 5 | Invalid secondary-header type, size, or content. |
| `INVALID_APPLICATION_DATA` | 6 | Invalid or oversized application data. |
| `NULL_POINTER` | 7 | A required pointer was null. |
| `INVALID_CHECKSUM` | 8 | The configured CRC16 validation failed. |
| `VALIDATION_FAILURE` | 9 | A validation operation rejected packet or stream state. |
| `TEMPLATE_SET_FAILURE` | 10 | A Manager template could not be installed. |
| `FILE_READ_ERROR` | 11 | Input could not be read. |
| `FILE_WRITE_ERROR` | 12 | Output could not be written. |
| `CONFIG_FILE_ERROR` | 13 | A configuration file, key, type, or value is invalid. |

## Stable symbolic names

`ccsds::errorCodeName()` returns a static symbolic label for every `ErrorCode` and performs no allocation:

```cpp
const auto result = ccsds::buffer::deserializeBounded(packet, rxBuffer, rxSize);
if (!result) {
  log(ccsds::errorCodeName(result.error().code()),
      result.error().message());
}
```

Examples include `INVALID_DATA`, `INVALID_CHECKSUM`, and `CONFIG_FILE_ERROR`. An unrecognized numeric enum value returns `UNRECOGNIZED_ERROR_CODE`.

The helper is intended for stable logs, telemetry diagnostics, test output, and bare-metal code that needs something more descriptive than the numeric category. The operation-specific `Error::message()` remains the detailed diagnostic and may change as diagnostics improve.

## Structured Validator reports

`ccsds::Validator::validate()` is intentionally different from an operation that
can fail to execute. It returns a `ccsds::ValidationReport` containing named
`ValidationCode` checks:

```cpp
const auto report = validator.validate(packet);
if (report.failed(ccsds::ValidationCode::Crc16)) {
  // CRC validation was performed and failed.
}
```

A report is not an `Error` and does not use numeric report positions. This lets
bare-metal code branch on stable enum values without allocating error strings or
depending on a six-element boolean-vector layout.

Use `ccsds::validationCodeName()` when a symbolic Validator-check label is needed.

Malformed wire input can still return a normal `Error` from parsing before a
Packet exists to pass to the Validator.

## Result aliases

```cpp
using ResultBool = Result<bool>;
using ResultBuffer = Result<std::vector<std::uint8_t>>;
```

## Propagation helpers

`CCSDSResult.h` defines propagation helpers used by existing library code,
including `RET_IF_ERR_MSG`, `ASSIGN_MV`, `ASSIGN_CP`, and `FORWARD_RESULT`.
Application code is usually clearer when it checks `Result<T>` explicitly at API
boundaries.

## Hosted and MCU builds

The same Result types, `errorCodeName()`, raw-buffer adapters, and structured Validator API are available in host and `CCSDS_MCU` builds. The project remains C++17 and supports MCU builds with `-fno-exceptions -fno-rtti`.
