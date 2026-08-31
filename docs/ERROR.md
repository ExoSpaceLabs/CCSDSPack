<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Error and Result handling

CCSDSPack reports checked operation failures through `ccsds::Result<T>` and `ccsds::Error`. Public packet and Manager operations use Result values rather than exceptions as their normal error channel.

## Basic pattern

```cpp
const auto result = packet.deserializeBounded(bytes);
if (!result) {
  log(result.error().code(), result.error().message());
  return result.error().code();
}

const std::size_t consumed = result.value();
```

A `Result<T>` contains either a success value or an `Error`; callers inspect the result before accessing `value()` or `error()`.

## Error categories

| Name | Value | Meaning |
|---|---:|---|
| `NONE` | 0 | No error |
| `UNKNOWN_ERROR` | 1 | Unclassified failure |
| `NO_DATA` | 2 | Required data or stored packets are absent |
| `INVALID_DATA` | 3 | Malformed data or invalid boundary |
| `INVALID_HEADER_DATA` | 4 | Invalid primary-header state |
| `INVALID_SECONDARY_HEADER_DATA` | 5 | Invalid secondary-header type, size, or content |
| `INVALID_APPLICATION_DATA` | 6 | Invalid or oversized application data |
| `NULL_POINTER` | 7 | Required pointer is null |
| `INVALID_CHECKSUM` | 8 | CRC16 verification failed |
| `VALIDATION_FAILURE` | 9 | Packet or stream state failed validation |
| `TEMPLATE_SET_FAILURE` | 10 | Manager template installation failed |
| `FILE_READ_ERROR` | 11 | Input read failed |
| `FILE_WRITE_ERROR` | 12 | Output write failed |
| `CONFIG_FILE_ERROR` | 13 | Configuration file/key/type/value is invalid |

## Symbolic names

`ccsds::errorCodeName()` returns a static symbolic label without allocation. `Error::message()` provides operation-specific detail.

```cpp
if (!result) {
  log(ccsds::errorCodeName(result.error().code()),
      result.error().message());
}
```

## Structured validation

`ccsds::Validator::validate()` returns a `ValidationReport`, not an operation `Error`. The report uses stable named `ValidationCode` entries so application and embedded code can branch on a specific failed check.

```cpp
const auto report = validator.validate(packet);
if (report.failed(ccsds::ValidationCode::Crc16)) {
  handleBadCrc();
}
```

Use `ccsds::validationCodeName()` for a symbolic check label. Malformed wire input can still return a parsing `Error` before a Packet exists for object validation.

## Result aliases

```cpp
using ResultBool = Result<bool>;
using ResultBuffer = Result<std::vector<std::uint8_t>>;
```

The same Result/Error and Validator APIs are available in hosted and `CCSDS_MCU` C++17 builds, including builds that disable RTTI and exceptions.
