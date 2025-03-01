#ifndef TESTMANAGER_H
#define TESTMANAGER_H

#include <string>
#include <chrono>

const std::string GREEN = "\033[32m"; ///< print Green color after this
const std::string   RED = "\033[31m"; ///< print Red color after this
const std::string RESET =  "\033[0m"; ///< print Reset color after this

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

    /**
     * @brief Marks the start of a unit test.
     *
     * Initializes the timer and tracks whether this is the first test in a series.
     */
    void unitTestStart();

    /**
     * @brief Marks the end of a unit test and logs its result.
     *
     * @param condition The result of the test (true for pass, false for fail).
     * @param message A message describing the test.
     */
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
#endif // TESTMANAGER_H

