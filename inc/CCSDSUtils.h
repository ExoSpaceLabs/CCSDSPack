#ifndef CCSDSUTILS_H
#define CCSDSUTILS_H

#include <string>
#include <cstdint>
#include <chrono>
#include <functional>


// functions
std::string getBinaryString(uint32_t value, int bits);
std::string getBitsSpaces(int num);

// testerClass
class Tester {
public:
    Tester() = default;
    ~Tester() = default;

    int Result();

    /**
    *  standalone test
    */
    template <typename Callable >
    void unitTest(const std::string& message, Callable lambda){
        unitTestStart();
        unitTestEnd(lambda(),message);
    }


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
};
#endif // CCSDSUTILS_H

