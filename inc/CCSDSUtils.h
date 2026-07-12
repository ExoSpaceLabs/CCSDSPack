// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_UTILS_H
#define CCSDS_UTILS_H

#include <CCSDSPacket.h>
#include <CCSDSManager.h>
#include <string>
#include <vector>

/**
 * @brief Computes the CRC-16 checksum for a given data vector with configurable parameters.
 *
 * @param data A vector of bytes to compute the checksum for.
 * @param polynomial The polynomial used for the CRC calculation.
 * @param initialValue The initial value of the CRC register.
 * @param finalXorValue The final XOR value applied to the CRC result.
 * @return The computed 16-bit CRC value.
 */
std::uint16_t crc16(const std::vector<std::uint8_t> &data,
                    std::uint16_t polynomial = 0x1021,
                    std::uint16_t initialValue = 0xFFFF,
                    std::uint16_t finalXorValue = 0x0000);

namespace CCSDS {
  using ::crc16;
}

bool stringEndsWith(const std::string &str, const std::string &suffix);
void printPacket(CCSDS::Packet &packet);
void printPackets(CCSDS::Manager &manager);
std::string getBinaryString(std::uint32_t value, std::int32_t bits);
std::string getBitsSpaces(std::int32_t num);
void printBufferData(const std::vector<std::uint8_t> &buffer, std::int32_t limitBytes = 20);
void printData(CCSDS::DataField dataField);
void printHeader(CCSDS::Header &header);
CCSDS::ResultBool printPrimaryHeader(CCSDS::Packet &packet);
void printDataField(CCSDS::Packet &packet);
CCSDS::ResultBool writeBinaryFile(const std::vector<std::uint8_t> &data,
                                  const std::string &filename);
CCSDS::ResultBuffer readBinaryFile(const std::string &filename);
bool fileExists(const std::string &fileName);

#endif // CCSDS_UTILS_H
