

#include "CCSDSUtils.h"
#include <iostream>
#include <iomanip>

//###########################################################################
#define VERBOSE 1


std::string getBinaryString(const uint32_t value, const int bits) {
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

std::string getBitsSpaces(const int num){
    std::string spaces;
    
    for (int i = num - 1; i >= 0; --i) {
        spaces += ' ';
    }

    return spaces;
}

uint16_t crc16(
    const std::vector<uint8_t>& data, const uint16_t polynomial, const uint16_t initialValue, const uint16_t finalXorValue) {
    uint16_t crc = initialValue;

    for (const auto& byte : data) {
        crc ^= static_cast<uint16_t>(byte) << 8;  // Align byte with MSB of 16-bit CRC
        for (int i = 0; i < 8; ++i) {             // Process each bit
            if (crc & 0x8000) {                   // Check if MSB is set
                crc = (crc << 1) ^ polynomial;    // Shift and XOR with polynomial
            } else {
                crc = crc << 1;                   // Shift only
            }
        }
    }

    return crc ^ finalXorValue; // Apply final XOR
}


/**
 * @brief Marks the start of a unit test.
 *
 * Initializes the timer and tracks whether this is the first test in a series.
 */
void TestManager::unitTestStart() {
    if (!m_testStarted) {
        m_testStarted = true;
        m_startTime = std::chrono::high_resolution_clock::now();
        m_unitStartTime = m_startTime;
    }else {
        m_unitStartTime = std::chrono::high_resolution_clock::now();
    }
}

/**
 * @brief Marks the end of a unit test and logs its result.
 *
 * @param condition The result of the test (true for pass, false for fail).
 * @param message A message describing the test.
 */
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

