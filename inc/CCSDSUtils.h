#ifndef CCSDSUTILS_H
#define CCSDSUTILS_H

#include <string>
#include <cstdint>
#include <chrono>
#include <vector>

const std::string GREEN = "\033[32m"; ///< print Green color after this
const std::string   RED = "\033[31m"; ///< print Red color after this
const std::string RESET =  "\033[0m"; ///< print Reset color after this

// functions
/**
 * @brief Computes the CRC-16 checksum for a given data vector with configurable parameters.
 *
 * @param data A vector of bytes to compute the checksum for.
 * @param polynomial The polynomial used for the CRC calculation (default: CCSDS CRC-16 polynomial 0x1021).
 * @param initialValue The initial value of the CRC register (default: 0xFFFF).
 * @param finalXorValue The final XOR value applied to the CRC result (default: 0x0000).
 * @return The computed 16-bit CRC value.
 */
uint16_t crc16(const std::vector<uint8_t>& data,
    uint16_t polynomial = 0x1021,
    uint16_t initialValue = 0xFFFF,
    uint16_t finalXorValue = 0x0000
);

/**
 * @brief Converts a given value to its binary representation as a string, with spaces every 4 bits.
 *
 * @param value The 32-bit integer value to convert.
 * @param bits The number of significant bits to include in the binary string.
 * @return A string representing the binary value with spaces every 4 bits.
 */
std::string getBinaryString(uint32_t value, int bits);

/**
 * @brief Generates a string of spaces for formatting binary outputs.
 *
 * @param num The number of spaces required.
 * @return A string of spaces of length `num`.
 */
std::string getBitsSpaces(int num);

/**
 * @brief A utility class for testing and validation.
 *
 * This class provides methods for running unit tests with descriptive messages
 * and conditions. It encapsulates test management, logging, and result tracking
 * in a structured and reusable way.
 *
 * Example usage:
 * @code
 * TestManager tester;
 * tester.unitTest("Example Test", []() { return 2 + 2 == 4; });
 * tester.Result();
 * @endcode
 * @note The unitTest can method can be called as much as required, execution time
 * is calculated only for the code within the lambda.
 *
 * @throws Nothing in this case.
 */
class TestManager {
public:
    TestManager() = default;
    ~TestManager() = default;

    /**
     * @brief Runs a unit test with a given message and condition.
     *
     * Executes a lambda function that represents the test condition and logs
     * the unit tests result alongside a descriptive message.
     *
     * @param message A descriptive message for the unit test.
     * @param condition A lambda function or callable object that evaluates to a boolean.
     *                  It represents the test condition and returns `true` if the test passes.
     *
     * @return none.
     */
    template <typename Callable >
    void unitTest(const std::string& message, Callable condition){
        unitTestStart();
        unitTestEnd(condition(),message);
    }
    /**
     * @brief Marks the end of the tests and logs the result to the console.
     *
     * @note The total execution time is calculated from when the first
     * unit test has been initiated until this functions execution.
     *
     * @return integer representing the number of tests failed, 0 if all passed.
     */
    int Result();

private:
    void unitTestStart();
    void unitTestEnd(bool condition, const std::string& message);


    size_t m_testCounter{};
    size_t m_testsFailed{};
    size_t m_testsPassed{};
    bool m_testStarted{false};
    std::chrono::time_point<std::chrono::system_clock> m_startTime;
    std::chrono::time_point<std::chrono::system_clock> m_unitStartTime;
    std::chrono::time_point<std::chrono::system_clock> m_unitEndTime;
    double m_totalTime{};
};
#endif // CCSDSUTILS_H

