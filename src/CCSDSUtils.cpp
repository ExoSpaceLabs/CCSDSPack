

#include "CCSDSUtils.h"
#include <iostream>
#include <iomanip>

//###########################################################################
#define VERBOSE 1

std::string getBinaryString(uint32_t value, int bits) {
    std::string binaryString;
        // Calculate the minimum number of bits required to represent in groups of 4
    const int paddedBits = ((bits + 3) / 4) * 4;  // Round up to the nearest multiple of 4

    for (int i = paddedBits - 1; i >= 0; --i) {
        binaryString += ((value >> i) & 1) ? '1' : '0';

        // Add a space after every 4 bits, except at the end
        if (i % 4 == 0 && i != 0) {
            binaryString += ' ';
        }
    }
    return binaryString;
}

std::string getBitsSpaces(int num){
    std::string spaces;
    
    for (int i = num - 1; i >= 0; --i) {
        spaces += ' ';
    }

    return spaces;
}

void TestManager::unitTestStart() {
    if (!m_testStarted) {
        std::cout << std::endl;
        std::cout <<"Running Tests..." << std::endl;
        m_startTime = std::chrono::high_resolution_clock::now();
        m_unitStartTime = m_startTime;
        m_testStarted = true;
    }else {
        m_unitStartTime = std::chrono::high_resolution_clock::now();
    }
}


void TestManager::unitTestEnd(const bool condition, const std::string& message) {
    m_unitEndTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = m_unitEndTime - m_unitStartTime;
    const auto us = elapsed.count() * 1000000;
    m_testCounter++;
    if (condition) {
        m_testsPassed++;
#if VERBOSE == 1
        std::cout << "# "<< std::setw(3) << std::setfill(' ') << m_testCounter
        << " [ "<< GREEN << "PASS" << RESET << " | "<< std::setw(6) << std::setfill(' ') << us
        << " us ] Test: "<<  message << std::endl;
#endif
    }else {
        m_testsFailed++;
#if VERBOSE == 1
        std::cout << "# "<< std::setw(3) << std::setfill(' ') << m_testCounter
        << " [ " << RED << "FAIL" << RESET<< " | " <<  std::setw(6) << std::setfill(' ') << us
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
    std::cout << "  PASSED: [" << std::setw(spaceSize) << std::setfill(' ') << m_testsPassed << " ]"
              << "  FAILED: [" << std::setw(spaceSize) << std::setfill(' ') << m_testsFailed << " ]"
              << "   TOTAL: [" << std::setw(spaceSize) << std::setfill(' ') << m_testCounter << " ]"
              << "    TIME: [" << std::setw(spaceSize) << std::setfill(' ') << us << " us ]";
    std::cout << std::endl;
    return static_cast<int>(m_testsFailed);
}

