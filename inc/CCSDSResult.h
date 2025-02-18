//
// Created by dev on 2/16/25.
//

#ifndef CCSDSRESULT_H
#define CCSDSRESULT_H

#include <unordered_map>
#include <variant>
#include <vector>
#include <cstdint>

namespace CCSDS {

    // Define an enum for error codes
    enum ErrorCode : int32_t{
        NONE = 0,
        NO_DATA = -128,
        INVALID_DATA = -129,
        INVALID_CHECKSUM = -130,
        SOMETHING_WENT_WRONG = -131,
        UNKNOWN_ERROR = -132
    };

    // Define a static unordered map for error messages
    //todo remove the map, no need for this map, I can just log the error with cerr and return code.
    const std::unordered_map<ErrorCode, const char*> errorMessageMap = {
        {NONE, "No error"},
        {NO_DATA, "No Data"},
        {INVALID_DATA, "Invalid data provided"},
        {INVALID_CHECKSUM, "Invalid checksum detected"},
        {SOMETHING_WENT_WRONG, "Data packet is too short"},
        {UNKNOWN_ERROR, "An unknown error occurred"}
    };

    // Function to get error message from the map
    //todo therefore this might be redundant as well.
    inline const char* getErrorMessage(const ErrorCode code) {
        const auto it = errorMessageMap.find(code);
        return (it != errorMessageMap.end()) ? it->second : "Unhandled error";
    }



    // The `Result<T>` class encapsulating both a value and an error code
    template <typename T>
    class Result {
        std::variant<T, ErrorCode> data;  // Holds either a value or an error

    public:
        // Constructor for success case (T value)
        Result(T value) : data(std::move(value)) {}

        // Constructor for failure case (ErrorCode)
        Result(ErrorCode error) : data(error) {}

        // Check if the result contains a valid value
        [[nodiscard]] bool has_value() const{
            return std::holds_alternative<T>(data);
        }

        // Retrieve the value (assumes the caller checks `has_value()`)
        T& value() { return std::get<T>(data); }
        const T& value() const { return std::get<T>(data); }

        // Retrieve the error code
        [[nodiscard]] ErrorCode error() const{
            return std::get<ErrorCode>(data);
        }

        // Get human-readable error message
        [[nodiscard]] const char* error_message() const{
            return getErrorMessage(error());
        }

        // Implicit conversion to `bool`, allowing usage like `if (result)`
        explicit operator bool() const { return has_value(); }
    };

    // Define alias for convenience
    using ResultBool = Result<bool>;
    using ResultBuffer = Result<std::vector<uint8_t>>;

}

// MACROS FOR Result error management class.

#define RETURN_IF_ERROR(condition, errorCode)  \
do { if (condition) return errorCode; } while (0)

#define RET_IF_ERR_MSG(condition, errorCode, message)   \
do {                                                    \
    if (condition){                                     \
        std::cerr << "[ Error ]: Code ["<< errorCode << "]: " << message << '\n'; \
        return errorCode;                               \
    }                                                   \
} while (0)

#define ASSIGN_OR_RETURN(var, result)          \
    do {                                       \
        auto&& _res = (result);                \
    if (!_res) return Result<T>(_res.error()); \
        var = std::move(_res.value());         \
} while (0)

#define ASSIGN_OR_PRINT(var, result)           \
    do {                                       \
        auto&& _res = (result);                \
        if (!_res) {                           \
std::cerr << "[ Error ]: Code ["<< _res.error() << "]: " << CCSDS::errorMessageMap.at(_res.error()) << '\n'; \
        } else {                               \
            var = std::move(_res.value());     \
        }                                      \
    } while (0)


//used in unit testing to return false if an error occured.
#define TEST_RET(var, result)                  \
    do {                                       \
        auto&& _res = (result);                \
        if (!_res) {                           \
std::cerr << "[ Error ]: Code ["<< _res.error() << "]: " << CCSDS::errorMessageMap.at(_res.error()) << '\n'; return false; \
        } else {                               \
            var = std::move(_res.value());     \
        }                                      \
    } while (0)

//used in unit testing to return false if an error occured.
#define TEST_VOID( result )                    \
    do {                                       \
        auto&& _res = (result);                \
        if (!_res) {                           \
            std::cerr << "[ Error ]: Code ["<< _res.error() << "]: " << CCSDS::errorMessageMap.at(_res.error()) << '\n'; return false; \
        }                                      \
    } while (0)

// If `result` contains an error, return immediately (for void functions)
#define ASSERT_SUCCESS(result)                 \
    do {                                       \
        auto&& _res = (result);                \
        if (!_res.has_value()) return;         \
    } while (0)

// Forward the result as-is (for functions returning Result<T>)
#define FORWARD_RESULT(result)                 \
    do {                                       \
        auto&& _res = (result);                \
        if (!_res.has_value()) return _res;    \
    } while (0)

#endif //CCSDSRESULT_H
