#ifndef CCSDSUTILS_H
#define CCSDSUTILS_H

#include <CCSDSPacket.h>
#include <string>
#include <cstdint>
#include <vector>

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


void printPackets(std::vector<CCSDS::Packet>& packets);

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

void printBufferData(const std::vector<uint8_t>& buffer);
void printData(CCSDS::DataField dataField);
void printHeader(CCSDS::Header& header);

CCSDS::ResultBool printPrimaryHeader(CCSDS::Packet &packet);
void printDataField(CCSDS::Packet& packet);

#endif // CCSDSUTILS_H

