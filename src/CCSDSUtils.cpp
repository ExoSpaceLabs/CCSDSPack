#include "CCSDSUtils.h"
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <fstream>

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

void printBufferData(const std::vector<uint8_t> &buffer, const int limitBytes) {
  std::cout << "[ ";
  if (buffer.size() > limitBytes) {
    for (size_t i= 0 ; i < static_cast<int>(limitBytes / 2); i++) {
      std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(buffer[i]) << " ";
    }
    std::cout << "... ";
    for (size_t i = buffer.size() - static_cast<int>(limitBytes / 2) ; i < buffer.size(); i++) {
      std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(buffer[i]) << " ";
    }
  } else {
    for (const unsigned char i: buffer) {
      std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(i) << " ";
    }
  }
  std::cout << "]" << std::endl;
}

void printData(CCSDS::DataField dataField) {
  const auto dataFieldHeader = dataField.getDataFieldHeaderBytes();
  auto applicationData = dataField.getApplicationData();
  const uint16_t maxSize = (applicationData.size() > dataFieldHeader.size())
                             ? applicationData.size()
                             : dataFieldHeader.size();

  std::cout << " [CCSDS DATA] Data Field Length              : " << applicationData.size() + dataFieldHeader.size() << " bytes" << std::endl;
  std::cout << " [CCSDS DATA] Secondary Header Present       : [ " << (dataField.getDataFieldHeaderFlag()
                                                                         ? "True"
                                                                         : "False") << " ]" << std::endl;
  if (!dataFieldHeader.empty()) {
    std::cout << " [CCSDS DATA] Secondary Header         [Hex] : " << getBitsSpaces(
    (maxSize - static_cast<uint16_t>(dataFieldHeader.size())) * 4);
    printBufferData(dataFieldHeader);
  }

  std::cout << " [CCSDS DATA] Application Data         [Hex] : ";
  printBufferData(applicationData);
  std::cout << std::endl;
}

void printHeader(CCSDS::Header &header) {
  std::cout << " [CCSDS HEADER] Full Primary Header    [Hex] : [ " << getBitsSpaces(17 - 12) << "0x" << std::hex <<
      header.getFullHeader() << " ]" << std::endl;
  std::cout << std::endl;

  std::cout << " [CCSDS HEADER] Version Number               : [ " << getBitsSpaces(19 - 4) <<
      getBinaryString(header.getVersionNumber(), 3) << " ]";
  std::cout << " - [Dec] : "<< std::dec <<  static_cast<int>( header.getVersionNumber())  << std::endl;

  std::cout << " [CCSDS HEADER] Type                         : [ " << getBitsSpaces(19 - 4) <<
      getBinaryString(header.getType(), 1) << " ]";
  std::cout << " - [Dec] : "<< std::dec << static_cast<int>( header.getType()) << std::endl;

  std::cout << " [CCSDS HEADER] APID                         : [ " << getBitsSpaces(17 - 12) <<
    getBinaryString(header.getAPID(), 11) << " ]";
  std::cout << " - [Dec] : " << std::dec << header.getAPID() << std::endl;

  std::cout << " [CCSDS HEADER] Data Field Header Flag       : [ " << getBitsSpaces(19 - 4) << getBinaryString(
  header.getDataFieldHeaderFlag(), 1) << " ]";
  std::cout << " -       : " << (header.getDataFieldHeaderFlag() ? "True" : "False")  << std::endl;

  std::cout << " [CCSDS HEADER] Sequence Flags               : [ " << getBitsSpaces(19 - 4) <<
    getBinaryString(header.getSequenceFlags(), 2) << " ]";
  std::cout << " -       : ";
  switch(header.getSequenceFlags()) {
    case 0:
      std::cout << "CONTINUING_SEGMENT";
      break;
    case 1:
      std::cout << "FIRST_SEGMENT";
      break;
    case 2:
      std::cout << "LAST_SEGMENT";
      break;
    case 3:
      std::cout << "UNSEGMENTED";
      break;
    default:
      break;
  }
  std::cout << std::endl;

  std::cout << " [CCSDS HEADER] Sequence Count               : [ " << getBitsSpaces(0) << getBinaryString(
  header.getSequenceCount(), 14) << " ]";
  std::cout << " - [Dec] : "<< std::dec << header.getSequenceCount() << std::endl;

  std::cout << " [CCSDS HEADER] DataLength                   : [ " << getBinaryString(header.getDataLength(), 16) <<
    " ]";
  std::cout << " - [Dec] : "<< std::dec << header.getDataLength() << std::endl;

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
  std::cout << "[ CCSDSPack ] CRC-16                   [Hex] : [ " << "0x" << std::hex << packet.getCRC() << " ]";
  std::cout << " - [Dec] : "<< std::dec << packet.getCRC() << std::endl;

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

void printPacket(CCSDS::Packet &packet) {
  printPrimaryHeader(packet);
  printDataField(packet);
}

void printPackets(CCSDS::Manager& manager) {
  std::cout << "[ CCSDS Manager ] Number of Packets    : " << manager.getTotalPackets() << std::endl;
  std::cout << "[ CCSDS Manager ] Sync Pattern Enabled : " << (manager.getSyncPatternEnable() ? "True" : "False") << std::endl;
  std::cout << "[ CCSDS Manager ] Sync Pattern         : 0x" << std::hex << manager.getSyncPattern() << std::dec << std::endl;

  auto templatePacket = manager.getTemplate();
  std::cout << "[ CCSDS Manager ] Template             : " << std::endl ;
  printPrimaryHeader(templatePacket);

  int idx = 1;
  for (auto &packet: manager.getPacketsReference()) {
    std::cout << "__________________________________________________________________________________________________________________" << std::endl;
    std::cout << "[ CCSDS Manager ] Printing Packet [ " << idx << " ]:" << std::endl;
    std::cout << "[ CCSDS Manager ] Packet Length : " << packet.getFullPacketLength() << " bytes" << std::endl;
    std::cout << "[ CCSDS Manager ] Data ";
    printBufferData(packet.serialize(), 20);
    printPacket(packet);
    idx++;
  }
}

CCSDS::ResultBool writeBinaryFile(const std::vector<uint8_t>& data, const std::string& filename) {
  RET_IF_ERR_MSG(filename.empty(),CCSDS::ErrorCode::FILE_WRITE_ERROR, "No filename provided");
  RET_IF_ERR_MSG(data.empty(),CCSDS::ErrorCode::FILE_WRITE_ERROR, "No data provided");
  std::ofstream out(filename, std::ios::binary);

  RET_IF_ERR_MSG(!out,CCSDS::ErrorCode::FILE_WRITE_ERROR, "Failed to open file for writing");

  // Write the entire vector data to the file in one go
  out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  RET_IF_ERR_MSG(!out,CCSDS::ErrorCode::FILE_WRITE_ERROR, "Failed to write the data to the file");

  return true;
}

CCSDS::ResultBuffer readBinaryFile(const std::string& filename) {
  RET_IF_ERR_MSG(filename.empty(),CCSDS::ErrorCode::FILE_READ_ERROR, "No filename provided");

  std::ifstream in(filename, std::ios::binary | std::ios::ate);
  RET_IF_ERR_MSG(!in,CCSDS::ErrorCode::FILE_READ_ERROR, "Failed to open file for reading");

  // Get the file size using the 'ate' flag (seeks to the end automatically)
  const std::streamsize size = in.tellg();
  in.seekg(0, std::ios::beg);

  // Read the entire file content into the vector
  std::vector<uint8_t> data(size);
  in.read(reinterpret_cast<char*>(data.data()), size);

  RET_IF_ERR_MSG(!in,CCSDS::ErrorCode::FILE_READ_ERROR, "Failed to read the entire file");
  return data;
}

bool fileExists(const std::string &fileName) {
  auto res = readBinaryFile(fileName);
  if ( res.has_value()) {
    return true;
  }
  return false;
}

bool stringEndsWith(const std::string& str, const std::string& suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
