// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSUtils.h
 * @brief Declares CRC, diagnostic formatting, and binary file helper functions.
 */
#ifndef CCSDS_UTILS_H
#define CCSDS_UTILS_H

#include <CCSDSPacket.h>
#include <CCSDSManager.h>
#include <string>
#include <vector>

/**
 * @brief Computes an MSB-first 16-bit CRC over the supplied bytes.
 * @param data Bytes processed in order.
 * @param polynomial Generator polynomial; defaults to 0x1021.
 * @param initialValue Initial register value; defaults to 0xFFFF.
 * @param finalXorValue Value XORed with the final register; defaults to zero.
 * @return Calculated 16-bit CRC.
 *
 * The default parameters implement CRC-16/CCITT-FALSE. Packet uses this function
 * over finalized primary-header and packet-data-field bytes, excluding the CRC itself.
 */
std::uint16_t crc16(const std::vector<std::uint8_t> &data,
                    std::uint16_t polynomial = 0x1021,
                    std::uint16_t initialValue = 0xFFFF,
                    std::uint16_t finalXorValue = 0x0000);

namespace CCSDS {
  /** @brief Makes the global crc16() helper available as CCSDS::crc16. */
  using ::crc16;
}

/**
 * @brief Tests whether a string ends with an exact suffix.
 * @param str Input string.
 * @param suffix Suffix to compare.
 * @return True when suffix occurs at the end of str.
 */
bool stringEndsWith(const std::string &str, const std::string &suffix);

/**
 * @brief Prints a human-readable summary of one packet to standard output.
 * @param packet Packet to inspect; current stored state is printed.
 */
void printPacket(CCSDS::Packet &packet);

/**
 * @brief Prints all packets currently stored in a Manager.
 * @param manager Manager to inspect.
 */
void printPackets(CCSDS::Manager &manager);

/**
 * @brief Formats the low-order bits of an integer as a binary string.
 * @param value Value to format.
 * @param bits Number of low-order bits to include.
 * @return Binary string without a `0b` prefix.
 */
std::string getBinaryString(std::uint32_t value, std::int32_t bits);

/**
 * @brief Returns spacing used by the diagnostic bit-field formatter.
 * @param num Number of spaces.
 * @return String containing num spaces, or an empty string for non-positive input.
 */
std::string getBitsSpaces(std::int32_t num);

/**
 * @brief Prints bytes in hexadecimal form to standard output.
 * @param buffer Bytes to print.
 * @param limitBytes Maximum bytes displayed; defaults to 20.
 */
void printBufferData(const std::vector<std::uint8_t> &buffer, std::int32_t limitBytes = 20);

/**
 * @brief Prints a DataField summary.
 * @param dataField DataField copied for diagnostic output.
 */
void printData(CCSDS::DataField dataField);

/**
 * @brief Prints a primary-header summary.
 * @param header Header to inspect.
 */
void printHeader(CCSDS::Header &header);

/**
 * @brief Prints the primary header owned by a packet.
 * @param packet Packet to inspect.
 * @return Success, or an error when the header cannot be represented.
 */
CCSDS::ResultBool printPrimaryHeader(CCSDS::Packet &packet);

/**
 * @brief Prints the secondary-header and application-data portions of a packet.
 * @param packet Packet to inspect.
 */
void printDataField(CCSDS::Packet &packet);

/**
 * @brief Writes bytes to a binary file, replacing any existing content.
 * @param data Bytes to write.
 * @param filename Output path.
 * @return Success, or FILE_WRITE_ERROR.
 */
CCSDS::ResultBool writeBinaryFile(const std::vector<std::uint8_t> &data,
                                  const std::string &filename);

/**
 * @brief Reads an entire binary file into memory.
 * @param filename Input path.
 * @return File bytes, or FILE_READ_ERROR.
 */
CCSDS::ResultBuffer readBinaryFile(const std::string &filename);

/**
 * @brief Tests whether a file can be opened for reading.
 * @param fileName Path to test.
 * @return True when the file exists and is accessible.
 */
bool fileExists(const std::string &fileName);

#endif // CCSDS_UTILS_H
