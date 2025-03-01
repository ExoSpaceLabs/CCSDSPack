#ifndef CCSDSRESULT_H
#define CCSDSRESULT_H

#include <unordered_map>
#include <variant>
#include <vector>
#include <cstdint>
#include <iostream>

/**
 * @namespace CCSDS
 * @brief Contains result handling utilities for CCSDS packet processing.
 */
namespace CCSDS {

    /**
     * @enum ErrorCode
     * @brief Defines various error codes used in CCSDS packet handling.
     */
    enum ErrorCode : uint8_t {
        NONE = 0,                       ///< No error
        NO_DATA = 128,                  ///< No data available
        INVALID_DATA,                   ///< Data is invalid
        INVALID_HEADER_DATA,            ///< Header data is invalid
        INVALID_SECONDARY_HEADER_DATA,  ///< Secondary header data is invalid
        INVALID_APPLICATION_DATA,       ///< Application data is invalid
        NULL_POINTER,                   ///< Null pointer encountered
        INVALID_CHECKSUM,               ///< Checksum validation failed
        SOMETHING_WENT_WRONG,           ///< General failure
        UNKNOWN_ERROR                   ///< Unknown error
    };

    /**
     * @class Result
     * @brief Encapsulates a result that can hold either a value or an error code.
     *
     * This class simplifies error handling by allowing functions to return either a
     * valid result or an error code, reducing the need for exception handling.
     *
     * @tparam T The type of value to be stored in the result.
     */
    template <typename T>
    class Result {
        std::variant<T, ErrorCode> data; ///< Holds either a valid value or an error code.

    public:
        /**
         * @brief Constructor for success case.
         * @param value The successful result value.
         */
        Result(T value) : data(std::move(value)) {}

        /**
         * @brief Constructor for failure case.
         * @param error The error code.
         */
        Result(ErrorCode error) : data(error) {}

        /**
         * @brief Checks if the result contains a valid value.
         * @return True if a valid value is present, false otherwise.
         */
        [[nodiscard]] bool has_value() const {
            return std::holds_alternative<T>(data);
        }

        /**
         * @brief Retrieves the stored value.
         * @return A reference to the stored value.
         * @note Ensure `has_value()` returns true before calling this.
         */
        T& value() { return std::get<T>(data); }
        const T& value() const { return std::get<T>(data); }

        /**
         * @brief Retrieves the stored error code.
         * @return The error code.
         */
        [[nodiscard]] ErrorCode error() const {
            return std::get<ErrorCode>(data);
        }

        /**
         * @brief Implicit conversion to `bool`, allowing usage like `if (result)`.
         * @return True if the result contains a value, false otherwise.
         */
        explicit operator bool() const { return has_value(); }
    };

    // Convenient type aliases for common result types
    using ResultBool = Result<bool>;
    using ResultBuffer = Result<std::vector<uint8_t>>;

}

/**
 * @def RETURN_IF_ERROR(condition, errorCode)
 * @brief Macro to return an error code if a condition is met.
 */
#define RETURN_IF_ERROR(condition, errorCode)  \
do { if (condition) return errorCode; } while (0)

/**
 * @def RET_IF_ERR_MSG(condition, errorCode, message)
 * @brief Macro to return an error code with an error message if a condition is met.
 */
#define RET_IF_ERR_MSG(condition, errorCode, message)   \
do {                                                    \
    if (condition) {                                     \
        std::cerr << "[ Error ]: Code [" << errorCode << "]: " << message << '\n'; \
        return errorCode;                                \
    }                                                   \
} while (0)

/**
 * @def ASSIGN_MV(var, result)
 * @brief Macro to assign a result value to a variable or return an error by moving.
 */
#define ASSIGN_MV(var, result)          \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) return Result<T>(_res.error()); \
    var = std::move(_res.value());             \
} while (0)

/**
 * @def ASSIGN_CP(var, result)
 * @brief Macro to assign a result value to a variable or return an error by moving.
 */
#define ASSIGN_CP(var, result)          \
do {                                           \
auto&& _res = (result);                    \
if (!_res) return Result<T>(_res.error()); \
var = _res.value();             \
} while (0)


/**
 * @def ASSIGN_OR_PRINT(var, result)
 * @brief Macro to assign a result value or print an error message.
 */
#define ASSIGN_OR_PRINT(var, result)           \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) {                               \
        std::cerr << "[ Error ]: Code [" << _res.error() << "]: " << '\n'; \
    } else {                                   \
        var = std::move(_res.value());         \
    }                                          \
} while (0)

/**
 * @def TEST_RET(var, result)
 * @brief Macro used in unit testing to return false if an error occurs.
 */
#define TEST_RET(var, result)                  \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) {                               \
        std::cerr << "[ Error ]: Code [" << _res.error() << "]: " << '\n'; \
        return false;                          \
    } else {                                   \
        var = std::move(_res.value());         \
    }                                          \
} while (0)

/**
 * @def TEST_VOID(result)
 * @brief Macro used in unit testing to return false if an error occurs.
 */
#define TEST_VOID(result)                      \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res) {                               \
        std::cerr << "[ Error ]: Code [" << _res.error() << "]: " << "Test Void Failure Detected" << '\n'; \
        return false;                          \
    }                                          \
} while (0)

/**
 * @def ASSERT_SUCCESS(result)
 * @brief Macro to return immediately if the result contains an error (for void functions).
 */
#define ASSERT_SUCCESS(result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res.has_value()) return;             \
} while (0)

/**
 * @def FORWARD_RESULT(result)
 * @brief Macro to return a result as-is (for functions returning Result<T>).
 */
#define FORWARD_RESULT(result)                 \
do {                                           \
    auto&& _res = (result);                    \
    if (!_res.has_value()) return _res;        \
} while (0)

#endif // CCSDSRESULT_H
