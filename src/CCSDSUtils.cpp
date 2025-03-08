#include "CCSDSUtils.h"
#include <iostream>
#include <iomanip>

//###########################################################################
#define VERBOSE 1


std::string getBinaryString(const uint32_t value, const int bits) {
  std::string binaryString;
  // Calculate the minimum number of bits required to represent in groups of 4
  const int paddedBits = ((bits + 3) / 4) * 4; // Round up to the nearest multiple of 4

  for (int i = paddedBits - 1; i >= 0; --i) {
    binaryString += ((value >> i) & 1) ? '1' : '0';

    // Add a space after every 4 bits, except at the end
    if (i % 4 == 0 && i != 0) {
      binaryString += ' ';
    }
  }
  return binaryString;
}

std::string getBitsSpaces(const int num) {
  std::string spaces;

  for (int i = num - 1; i >= 0; --i) {
    spaces += ' ';
  }

  return spaces;
}

void printBufferData(const std::vector<uint8_t> &buffer) {
  std::cout << "[ ";
  for (const unsigned char i: buffer) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(i) << " ";
  }
  std::cout << "]" << std::endl;
}

void printData(CCSDS::DataField dataField) {
  const auto dataFieldHeader = dataField.getDataFieldHeaderBytes();
  auto applicationData = dataField.getApplicationData();
  const uint16_t maxSize = (applicationData.size() > dataFieldHeader.size())
                             ? applicationData.size()
                             : dataFieldHeader.size();

  std::cout << std::endl;
  std::cout << " [CCSDS DATA] Test result:" << std::endl;
  std::cout << " [CCSDS DATA] Secondary Header Present       : [ " << (dataField.getDataFieldHeaderFlag()
                                                                         ? "True"
                                                                         : "False") << " ]" << std::endl;
  std::cout << " [CCSDS DATA] Secondary Header         [Hex] : [ " << getBitsSpaces(
    (maxSize - static_cast<uint16_t>(dataFieldHeader.size())) * 4);
  for (const auto &data: dataFieldHeader) {
    std::cout << "0x" << std::hex << static_cast<unsigned int>(data) << " ";
  }

  std::cout << "]" << std::endl;
  std::cout << " [CCSDS DATA] Data Field               [Hex] : [ " << getBitsSpaces(
    (maxSize - static_cast<uint16_t>(applicationData.size())) * 4);
  for (const auto &data: applicationData) {
    std::cout << "0x" << std::hex << static_cast<unsigned int>(data) << " ";
  }
  std::cout << "]" << std::endl;
  std::cout << std::endl;
}

void printHeader(CCSDS::Header &header) {
  std::cout << std::endl;
  std::cout << " [CCSDS HEADER] Test result:" << std::endl;
  std::cout << " [CCSDS HEADER] Full Primary Header    [Hex] : [ " << getBitsSpaces(17 - 12) << "0x" << std::hex <<
      header.getFullHeader() << " ]" << std::endl;
  std::cout << std::endl;
  std::cout << " [CCSDS HEADER] Info: Version Number         : [ " << getBitsSpaces(19 - 4) <<
      getBinaryString(header.getVersionNumber(), 3) << " ]" << std::endl;
  std::cout << " [CCSDS HEADER] Info: Type                   : [ " << getBitsSpaces(19 - 4) <<
      getBinaryString(header.getType(), 1) << " ]" << std::endl;
  std::cout << " [CCSDS HEADER] Info: Data Field Header Flag : [ " << getBitsSpaces(19 - 4) << getBinaryString(
    header.getDataFieldHeaderFlag(), 1) << " ]" << std::endl;
  std::cout << " [CCSDS HEADER] Info: APID                   : [ " << getBitsSpaces(17 - 12) <<
      getBinaryString(header.getAPID(), 11) << " ]" << std::endl;
  std::cout << " [CCSDS HEADER] Info: Sequence Flags         : [ " << getBitsSpaces(19 - 4) <<
      getBinaryString(header.getSequenceFlags(), 2) << " ]" << std::endl;
  std::cout << " [CCSDS HEADER] Info: Sequence Count         : [ " << getBitsSpaces(0) << getBinaryString(
    header.getSequenceCount(), 14) << " ]" << std::endl;
  std::cout << " [CCSDS HEADER] Info: DataLength             : [ " << getBinaryString(header.getDataLength(), 16) <<
      " ]" << std::endl;
  std::cout << std::endl;
}


CCSDS::ResultBool printPrimaryHeader(CCSDS::Packet &packet) {
  CCSDS::Header header;
  FORWARD_RESULT(header.deserialize({packet.getPrimaryHeaderBytes()}));
  printHeader(header);
  return true;
}

void printDataField(CCSDS::Packet &packet) {
  auto dataField = packet.getDataField();

  printData(dataField);
  std::cout << "[ CCSDSPack ] CRC-16                   [Hex] : [ " << "0x" << std::hex << packet.getCRC() << " ]" <<
      std::endl;
}

uint16_t crc16(
  const std::vector<uint8_t> &data, const uint16_t polynomial, const uint16_t initialValue,
  const uint16_t finalXorValue) {
  uint16_t crc = initialValue;

  for (const auto &byte: data) {
    crc ^= static_cast<uint16_t>(byte) << 8; // Align byte with MSB of 16-bit CRC
    for (int i = 0; i < 8; ++i) {
      // Process each bit
      if (crc & 0x8000) {
        // Check if MSB is set
        crc = (crc << 1) ^ polynomial; // Shift and XOR with polynomial
      } else {
        crc = crc << 1; // Shift only
      }
    }
  }

  return crc ^ finalXorValue; // Apply final XOR
}

void printPackets(std::vector<CCSDS::Packet> &packets) {
  int idx = 0;
  for (auto &packet: packets) {
    std::cout << "[ CCSDS Manager ] Printing Packet [ " << idx << " ]:" << std::endl;
    std::cout << "[ CCSDS Manager ] Data ";
    printBufferData(packet.serialize());

    printPrimaryHeader(packet);
    printDataField(packet);

    idx++;
  }
}
