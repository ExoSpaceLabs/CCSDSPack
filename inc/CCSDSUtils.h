#ifndef CCSDS_UTILS_H
#define CCSDS_UTILS_H

#include <CCSDSPacket.h>
#include <CCSDSManager.h>
#include <string>
#include <vector>

// free functions
/**
 * @brief Computes the CRC-16 checksum for a given data vector with configurable parameters.
 *
 * @param data A vector of bytes to compute the checksum for.
 * @param polynomial The polynomial used for the CRC calculation (default: CCSDS CRC-16 polynomial 0x1021).
 * @param initialValue The initial value of the CRC register (default: 0xFFFF).
 * @param finalXorValue The final XOR value applied to the CRC result (default: 0x0000).
 * @return The computed 16-bit CRC value.
 */
uint16_t crc16(const std::vector<uint8_t> &data,
               uint16_t polynomial = 0x1021,
               uint16_t initialValue = 0xFFFF,
               uint16_t finalXorValue = 0x0000
);

/**
 * @brief Prints to console a  CCSDS Packets, breaking it down to Primary header and Data field.
 *
 * @param packet
 */
void printPacket(CCSDS::Packet &packet);

/**
 * @brief Prints to console a series of CCSDS Packets contained in the manager.
 *
 * @param manager
 */
void printPackets(CCSDS::Manager & manager);

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
 * @brief Prints to console the HEX data from the bytes vector.
 *
 * @param buffer
 * @param limitBytes
 */
void printBufferData(const std::vector<uint8_t> &buffer, int limitBytes = 20);

/**
 * @brief Prints the data field details, including the secondary header and application data.
 *
 * Outputs information about the presence of a secondary header and the content
 * of both the secondary header and the application data in hexadecimal format.
 *
 * @return none.
 */
void printData(CCSDS::DataField dataField);

/**
 * @brief Prints the header fields and their binary or hexadecimal representations.
 *
 * Outputs all relevant header fields, including the full primary header, version number,
 * type, data field header flag, APID, sequence flags, sequence count, and data length.
 * Each field is displayed with appropriate formatting and spacing.
 *
 * @return none.
 */
void printHeader(CCSDS::Header &header);

/**
 * @brief Prints to console the primary header of a provided CCSDS packet
 *
 * @param packet
 * @return ResultBool
 */
CCSDS::ResultBool printPrimaryHeader(CCSDS::Packet &packet);

/**
 * @brief Prints the data field and the CRC-16 checksum of the packet.
 *
 * Outputs the content of the data field and the CRC-16 checksum
 * in hexadecimal format to the standard output.
 *
 * @return none.
 */
void printDataField(CCSDS::Packet &packet);

/**
 * This function takes in a buffer of data and a file name. and writes the data in binary
 * form to the file.
 *
 * @param data vector of uint8_t
 * @param filename string
 * @return  Result boolean true if successful or error
 */
CCSDS::ResultBool writeBinaryFile(const std::vector<uint8_t>& data, const std::string& filename);

/**
 * Read a specified binary file and return its contents as a buffer
 *
 * @param filename
 * @return Result vector of uint8 or Error
 */
CCSDS::ResultBuffer readBinaryFile(const std::string& filename);

/**
 * @brief filesystem check fore file existence prepared for both windows and linux.
 *
 * @param fileName std::string
 * @return bool
 */
bool fileExists(const std::string &fileName);

/**
 * Tests if str ends with suffix.
 * equivalent to endsWith(str) in c++20
 *
 * @param str string
 * @param suffix string
 * @return boolean
 */
bool stringEndsWith(const std::string& str, const std::string& suffix);

#endif // CCSDS_UTILS_H