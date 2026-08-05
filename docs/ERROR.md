<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Error and Result handling

[Documentation index](README.md) | [API reference](https://exospacelabs.github.io/CCSDSPack/html/)

CCSDSPack reports checked failures through `ccsds::Result<T>` and `ccsds::Error`, defined in `inc/CCSDSResult.h`. Public packet and Manager operations do not use exceptions as their normal error channel.

## Basic pattern

```cpp
const auto result = packet.deserializeBounded(bytes);
if (!result) {
  std::cerr << "CCSDSPack error "
            << static_cast<unsigned>(result.error().code())
            << ": " << result.error().message() << '\n';
  return result.error().code();
}

const std::size_t consumed = result.value();
```

A `Result<T>` contains either a success value or an `Error`. Check `has_value()` or use the explicit boolean conversion before calling `value()` or `error()`.

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
| `VALIDATION_FAILURE` | 9 | The Validator rejected a packet or stream property. |
| `TEMPLATE_SET_FAILURE` | 10 | A Manager template could not be installed. |
| `FILE_READ_ERROR` | 11 | Input could not be read. |
| `FILE_WRITE_ERROR` | 12 | Output could not be written. |
| `CONFIG_FILE_ERROR` | 13 | A configuration file, key, type, or value is invalid. |

Branch on the error category and include `message()` in diagnostics. The message supplies operation-specific detail.

## Result aliases

```cpp
using ResultBool = Result<bool>;
using ResultBuffer = Result<std::vector<std::uint8_t>>;
```

## Propagation helpers

`CCSDSResult.h` also defines internal/publicly visible propagation macros used by existing v1 code:

- `RETURN_IF_ERROR`;
- `RET_IF_ERR_MSG`;
- `ASSIGN_MV` and `ASSIGN_CP`;
- `ASSIGN_OR_PRINT`;
- `ASSERT_SUCCESS`;
- `FORWARD_RESULT`.

Normal application code can remain clearer by checking `Result<T>` explicitly, particularly at API boundaries where logs and recovery policy belong to the caller.

## Hosted and MCU builds

The same result types are available in host and `CCSDS_MCU` builds. Requesting the inactive `std::variant` alternative is programmer misuse, so always check the result before accessing it even when the project is compiled with `-fno-exceptions`.
