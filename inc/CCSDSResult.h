//
// Created by dev on 2/16/25.
//

#ifndef CCSDSRESULT_H
#define CCSDSRESULT_H

#include <iostream>
#include <unordered_map>
#include <variant>

namespace CCSDS {

    // Define an enum for error codes
    enum class ErrorCode {
        NONE,
        NO_DATA,
        INVALID_DATA,
        INVALID_CHECKSUM,
        DATA_TOO_SHORT,
        UNKNOWN_ERROR
    };

    // Define a static unordered map for error messages
    const std::unordered_map<ErrorCode, const char*> errorMessageMap = {
        {ErrorCode::NONE, "No error"},
        {ErrorCode::NO_DATA, "No Data"},
        {ErrorCode::INVALID_DATA, "Invalid data provided"},
        {ErrorCode::INVALID_CHECKSUM, "Invalid checksum detected"},
        {ErrorCode::DATA_TOO_SHORT, "Data packet is too short"},
        {ErrorCode::UNKNOWN_ERROR, "An unknown error occurred"}
    };

    // Function to get error message from the map
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
}

#define RETURN_IF_ERROR(condition, errorCode)  \
do { if (condition) return errorCode; } while (0)

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
std::cerr << "Error: " << CCSDS::errorMessageMap.at(_res.error()) << '\n'; \
        } else {                               \
            var = std::move(_res.value());     \
        }                                      \
} while (0)


//used in unit testing to return false if an error occured.
#define TEST_ASSIGN_OR_RETURN_VALID(var, result) \
    do {                                       \
        auto&& _res = (result);                \
        if (!_res) {                           \
std::cerr << "Error: " << CCSDS::errorMessageMap.at(_res.error()) << '\n'; return false;\
        } else {                               \
            var = std::move(_res.value());     \
        }                                      \
} while (0)

#endif //CCSDSRESULT_H
