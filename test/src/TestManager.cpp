

#include "TestManager.h"
#include <iostream>
#include <iomanip>

//###########################################################################
#define VERBOSE 1

void TestManager::unitTestStart() {
    if (!m_testStarted) {
        m_testStarted = true;
        m_startTime = std::chrono::high_resolution_clock::now();
        m_unitStartTime = m_startTime;
    }else {
        m_unitStartTime = std::chrono::high_resolution_clock::now();
    }
}

void TestManager::unitTestEnd(const bool condition, const std::string& message) {
    m_unitEndTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = m_unitEndTime - m_unitStartTime;
    const auto us = elapsed.count() * 1000000;
    m_totalTime += us;
    m_testCounter++;
    if (condition) {
        m_testsPassed++;
#if VERBOSE == 1
        std::cout << "# "<< std::setw(3) << std::setfill(' ') << m_testCounter
        << " [ "<< GREEN << "PASS" << RESET << " | "<< std::setw(7) << std::setfill(' ') << us
        << " us ] Test: "<<  message << std::endl;
#endif
    }else {
        m_testsFailed++;
#if VERBOSE == 1
        std::cout << "# "<< std::setw(3) << std::setfill(' ') << m_testCounter
        << " [ " << RED << "FAIL" << RESET<< " | " <<  std::setw(7) << std::setfill(' ') << us
        << " us ] Test: "<<  message << std::endl;
#endif
    }
}

int TestManager::Result() {
    constexpr int spaceSize = 7;
    m_testStarted = false;
    const std::chrono::duration<double> elapsed = m_unitEndTime - m_startTime;
    const auto us = elapsed.count() * 1000000;

    std::cout <<"Test Results:" << std::endl;
    std::cout << "   PASSED:    [" << std::setw(spaceSize + 4) << std::setfill(' ') << m_testsPassed << " |"
              << std::setw(spaceSize + 4) << std::setfill(' ') << m_testsFailed
              << " ] :FAILED,     TOTAL: [" << std::setw(spaceSize) << std::setfill(' ') << m_testCounter << " ]\n"
              << "   TEST TIME: [ " << std::setw(spaceSize) << std::setfill(' ') << us << " us | "
              << std::setw(spaceSize) << std::setfill(' ') << m_totalTime << " us ] :ACTUAL TIME";
    std::cout << std::endl;
    return static_cast<int>(m_testsFailed);
}