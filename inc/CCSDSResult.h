// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSResult.h
 * @brief Defines non-throwing result/error types and propagation helpers used by CCSDSPack.
 */
#ifndef CCSDS_RESULT_H
#define CCSDS_RESULT_H

#include <variant>
#include <vector>
#include <cstdint>

#ifndef CCSDS_MCU
  #include <iostream>
#else
  #include <string>
#endif

namespace CCSDS {
  /**
   * @enum ErrorCode
   * @brief Stable categories returned by checked CCSDSPack operations.
   *
   * The accompanying Error::message() provides operation-specific detail. Callers
   * should normally branch on the category and include the message in diagnostics.
   */
  enum ErrorCode : std::uint8_t {
    NONE = 0,                           ///< No error.
    UNKNOWN_ERROR = 1,                  ///< Unclassified failure.
    NO_DATA = 2,                        ///< Required data or stored packets are absent.
    INVALID_DATA = 3,                   ///< Generic malformed data or invalid boundary.
    INVALID_HEADER_DATA = 4,            ///< Invalid CCSDS primary-header field or bytes.
    INVALID_SECONDARY_HEADER_DATA = 5,  ///< Invalid secondary-header type, size, or content.
    INVALID_APPLICATION_DATA = 6,       ///< Invalid or oversized application data.
    NULL_POINTER = 7,                   ///< A required input pointer was null.
    INVALID_CHECKSUM = 8,               ///< Packet CRC validation failed.
    VALIDATION_FAILURE = 9,             ///< Validator rejected a packet.
    TEMPLATE_SET_FAILURE = 10,          ///< Manager template could not be installed.
    FILE_READ_ERROR = 11,               ///< Input file could not be read.
    FILE_WRITE_ERROR = 12,              ///< Output file could not be written.
    CONFIG_FILE_ERROR = 13              ///< Configuration file/key/type/value is invalid.
  };

  /**
   * @class Error
   * @brief Immutable error category and diagnostic message returned by Result.
   */
  class Error {
  public:
    /**
     * @brief Constructs an error.
     * @param code Stable error category.
     * @param message Human-readable operation-specific diagnostic.
     */
    Error(const ErrorCode code, std::string message)
      : m_code(code), m_message(std::move(message)) {}

    /** @brief Returns the stable error category. */
    [[nodiscard]] ErrorCode code() const { return m_code; }

    /** @brief Returns the diagnostic message owned by this Error. */
    [[nodiscard]] const std::string &message() const { return m_message; }

  private:
    ErrorCode m_code;       ///< Stable error category.
    std::string m_message;  ///< Human-readable diagnostic.
  };

  /**
   * @class Result
   * @brief Holds either one successful value of type T or one Error.
   * @tparam T Success value type.
   *
   * Result is the library's exception-free error channel and is suitable for host
   * and MCU builds. Test it with has_value() or explicit bool conversion before
   * calling value() or error(); requesting the inactive variant throws
   * std::bad_variant_access on hosted implementations.
   *
   * @code{.cpp}
   * const auto result = packet.deserializeBounded(bytes);
   * if (!result) {
   *   log(result.error().message());
   *   return;
   * }
   * const std::size_t consumed = result.value();
   * @endcode
   */
  template<typename T>
  class Result {
    std::variant<T, Error> data; ///< Active success value or error.

  public:
    /**
     * @brief Constructs a successful result.
     * @param value Value moved into the result.
     */
    Result(T value) : data(std::move(value)) {}

    /**
     * @brief Constructs a failed result.
     * @param error Error copied into the result.
     */
    Result(Error error) : data(error) {}

    /** @brief Returns true when the success value is active. */
    [[nodiscard]] bool has_value() const {
      return std::holds_alternative<T>(data);
    }

    /**
     * @brief Returns mutable access to the success value.
     * @pre has_value() is true.
     */
    T &value() { return std::get<T>(data); }

    /**
     * @brief Returns read-only access to the success value.
     * @pre has_value() is true.
     */
    const T &value() const { return std::get<T>(data); }

    /**
     * @brief Returns a copy of the stored error.
     * @pre has_value() is false.
     */
    [[nodiscard]] Error error() const { return std::get<Error>(data); }

    /** @brief Explicitly converts to true for success and false for error. */
    explicit operator bool() const { return has_value(); }
  };

  /** @brief Common result type for operations whose success payload is boolean. */
  using ResultBool = Result<bool>;
  /** @brief Common result type for operations returning a byte buffer. */
  using ResultBuffer = Result<std::vector<std::uint8_t> >;
}

/**
 * @def RETURN_IF_ERROR
 * @brief Returns errorCode from the current function when condition is true.
 * @param condition Failure predicate evaluated once.
 * @param errorCode Value returned on failure.
 */
#define RETURN_IF_ERROR(condition, errorCode)            \
do { if (condition) return errorCode; } while (0)

/**
 * @def RET_IF_ERR_MSG
 * @brief Returns CCSDS::Error when condition is true.
 * @param condition Failure predicate evaluated once.
 * @param errorCode CCSDS::ErrorCode category.
 * @param message Diagnostic expression used to construct the Error.
 */
#define RET_IF_ERR_MSG(condition, errorCode, message)    \
do {                                                     \
    if (condition) {                                     \
        return CCSDS::Error{errorCode,message};          \
    }                                                    \
} while (0)

/**
 * @def ASSIGN_MV
 * @brief Propagates a failed Result or moves its success value into var.
 * @param var Assignment destination.
 * @param result Result expression evaluated once.
 */
#define ASSIGN_MV(var, result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) return _res.error();            \
    var = std::move(_res.value());             \
} while (0)

/**
 * @def ASSIGN_CP
 * @brief Propagates a failed Result or copies its success value into var.
 * @param var Assignment destination.
 * @param result Result expression evaluated once.
 */
#define ASSIGN_CP(var, result)                 \
do {                                           \
auto&& _res = (result);                        \
if (!_res) return _res.error();                \
var = _res.value();                            \
} while (0)

/**
 * @def ASSIGN_OR_PRINT
 * @brief Prints a failed Result or moves its success value into var.
 * @param var Assignment destination.
 * @param result Result expression evaluated once.
 * @note Intended for construction paths that cannot return Result, such as DataField setup.
 */
#define ASSIGN_OR_PRINT(var, result)           \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) {                               \
        printf("[ Error ]: Code [%u]: %s\n", static_cast<unsigned>(_res.error().code()), _res.error().message().c_str());; \
    } else {                                   \
        var = std::move(_res.value());         \
    }                                          \
} while (0)

/**
 * @def ASSERT_SUCCESS
 * @brief Returns from a void function when result contains an error.
 * @param result Result expression evaluated once.
 */
#define ASSERT_SUCCESS(result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res.has_value()) return;             \
} while (0)

/**
 * @def FORWARD_RESULT
 * @brief Returns a failed Result unchanged from the current Result-returning function.
 * @param result Result expression evaluated once.
 */
#define FORWARD_RESULT(result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res.has_value()) return _res;        \
} while (0)

#endif // CCSDS_RESULT_H
