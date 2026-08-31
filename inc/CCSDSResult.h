// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSResult.h
 * @brief Defines non-throwing result/error types and propagation helpers used by CCSDSPack.
 */
#ifndef CCSDS_RESULT_H
#define CCSDS_RESULT_H

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#ifndef CCSDS_MCU
  #include <iostream>
#endif

namespace ccsds {
  enum ErrorCode : std::uint8_t {
    NONE = 0,
    UNKNOWN_ERROR = 1,
    NO_DATA = 2,
    INVALID_DATA = 3,
    INVALID_HEADER_DATA = 4,
    INVALID_SECONDARY_HEADER_DATA = 5,
    INVALID_APPLICATION_DATA = 6,
    NULL_POINTER = 7,
    INVALID_CHECKSUM = 8,
    VALIDATION_FAILURE = 9,
    TEMPLATE_SET_FAILURE = 10,
    FILE_READ_ERROR = 11,
    FILE_WRITE_ERROR = 12,
    CONFIG_FILE_ERROR = 13
  };

  [[nodiscard]] inline const char *errorCodeName(const ErrorCode code) noexcept {
    switch (code) {
      case NONE: return "NONE";
      case UNKNOWN_ERROR: return "UNKNOWN_ERROR";
      case NO_DATA: return "NO_DATA";
      case INVALID_DATA: return "INVALID_DATA";
      case INVALID_HEADER_DATA: return "INVALID_HEADER_DATA";
      case INVALID_SECONDARY_HEADER_DATA: return "INVALID_SECONDARY_HEADER_DATA";
      case INVALID_APPLICATION_DATA: return "INVALID_APPLICATION_DATA";
      case NULL_POINTER: return "NULL_POINTER";
      case INVALID_CHECKSUM: return "INVALID_CHECKSUM";
      case VALIDATION_FAILURE: return "VALIDATION_FAILURE";
      case TEMPLATE_SET_FAILURE: return "TEMPLATE_SET_FAILURE";
      case FILE_READ_ERROR: return "FILE_READ_ERROR";
      case FILE_WRITE_ERROR: return "FILE_WRITE_ERROR";
      case CONFIG_FILE_ERROR: return "CONFIG_FILE_ERROR";
      default: return "UNRECOGNIZED_ERROR_CODE";
    }
  }

  class Error {
  public:
    Error(const ErrorCode code, std::string message)
      : m_code(code), m_message(std::move(message)) {}
    [[nodiscard]] ErrorCode code() const { return m_code; }
    [[nodiscard]] const std::string &message() const { return m_message; }
  private:
    ErrorCode m_code;
    std::string m_message;
  };

  template<typename T>
  class Result {
    std::variant<T, Error> data;
  public:
    Result(T value) : data(std::move(value)) {}
    Result(Error error) : data(error) {}
    [[nodiscard]] bool has_value() const { return std::holds_alternative<T>(data); }
    T &value() { return std::get<T>(data); }
    const T &value() const { return std::get<T>(data); }
    [[nodiscard]] Error error() const { return std::get<Error>(data); }
    explicit operator bool() const { return has_value(); }
  };

  using ResultBool = Result<bool>;
  using ResultBuffer = Result<std::vector<std::uint8_t> >;
}

#define RETURN_IF_ERROR(condition, errorCode)            \
do { if (condition) return errorCode; } while (0)

#define RET_IF_ERR_MSG(condition, errorCode, message)    \
do {                                                     \
    if (condition) {                                     \
        return ccsds::Error{errorCode,message};          \
    }                                                    \
} while (0)

#define ASSIGN_MV(var, result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) return _res.error();            \
    var = std::move(_res.value());             \
} while (0)

#define ASSIGN_CP(var, result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) return _res.error();            \
    var = _res.value();                        \
} while (0)

#define ASSIGN_OR_PRINT(var, result)           \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) {                               \
        printf("[ Error ]: Code [%u]: %s\n", static_cast<unsigned>(_res.error().code()), _res.error().message().c_str()); \
    } else {                                   \
        var = std::move(_res.value());         \
    }                                          \
} while (0)

#define ASSERT_SUCCESS(result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res.has_value()) return;             \
} while (0)

/**
 * @brief Propagates only the Error so the caller may return a different Result<T>.
 */
#define FORWARD_RESULT(result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res.has_value()) return _res.error();\
} while (0)

#endif // CCSDS_RESULT_H
