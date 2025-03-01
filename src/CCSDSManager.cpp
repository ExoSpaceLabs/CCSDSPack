
#include "CCSDSManager.h"
#include <utility>
#include "CCSDSUtils.h"

void CCSDS::Manager::setPacketTemplate(CCSDS::Packet packet){
  m_packetTemplate = std::move(packet);
}

void CCSDS::Manager::setDatFieldSize(const uint16_t size) {
  m_packetTemplate.setDataFieldSize(size);
}

CCSDS::ResultBool CCSDS::Manager::setApplicationData(const std::vector<uint8_t>& data) {
   RET_IF_ERR_MSG(data.empty(), ErrorCode::NO_DATA,"Cannot set Application data, Provided data is empty");
  if (!m_packets.empty()) {
    m_packets.clear();
  }
  const auto maxBytesPerPacket = m_packetTemplate.getDataFieldMaximumSize();
  const auto dataBytesSize = data.size();
  int i = 0;
  auto remainderBytes = static_cast<int>( dataBytesSize );
  auto sequenceFlag = UNSEGMENTED;
  uint16_t sequenceCount = 1;
  while (i < dataBytesSize) {
    Packet newPacket = m_packetTemplate;
    std::vector<uint8_t> tmp;
    if (remainderBytes > maxBytesPerPacket) {
      tmp.insert(tmp.end(), data.begin() + i, data.begin() + i + maxBytesPerPacket);
      remainderBytes -= maxBytesPerPacket;
      if (i == 0) {
        sequenceFlag = FIRST_SEGMENT;
      }else {
        sequenceFlag = CONTINUING_SEGMENT;
      }
      i += maxBytesPerPacket;
      FORWARD_RESULT( newPacket.setApplicationData(tmp));
      newPacket.setSequenceCount(sequenceCount);
      newPacket.setSequenceFlags(sequenceFlag);
    } else {
      tmp.insert(tmp.end(), data.begin() + i, data.begin() + i + remainderBytes);
      i += remainderBytes;
      if ( sequenceFlag != UNSEGMENTED) {
        newPacket.setSequenceCount(sequenceCount);
        newPacket.setSequenceFlags(LAST_SEGMENT);
      }
      FORWARD_RESULT( newPacket.setApplicationData(tmp));
    }
    m_packets.push_back(std::move(newPacket));
    sequenceCount++;
  }
   return true;
}

void CCSDS::Manager::setAutoUpdateEnable(const bool enable) {
  m_updateEnable = enable;
  for (auto& packet : m_packets) {
    packet.setUpdatePacketEnable(enable);
  }
}

CCSDS::ResultBuffer CCSDS::Manager::getPacketTemplate() {
  auto data = m_packetTemplate.serialize();
   RET_IF_ERR_MSG(data.empty(), ErrorCode::NO_DATA,"Cannot get Packet template data, data is empty");
  return data;
}

CCSDS::ResultBuffer CCSDS::Manager::getPacketAtIndex(const uint16_t index) {
  RET_IF_ERR_MSG(index < 0 || index >= m_packets.size(), ErrorCode::INVALID_DATA, "Cannot get packet, index is out of bounds");
  return m_packets[index].serialize();
}

CCSDS::ResultBuffer CCSDS::Manager::getApplicationData() const {
   RET_IF_ERR_MSG(m_packets.empty(), ErrorCode::NO_DATA,"Cannot get Application data, no packets have been set."); // todo check if valid?
  std::vector<uint8_t> data;
  for (auto packet : m_packets) {
    auto applicationData = packet.getApplicationDataBytes();
    data.insert(data.end(),    applicationData.begin(),    applicationData.end());
  }
  return data;
}

CCSDS::ResultBuffer CCSDS::Manager::getApplicationDataAtIndex(const uint16_t index) {
  RET_IF_ERR_MSG(index < 0 || index >= m_packets.size(), ErrorCode::INVALID_DATA, "Cannot get Application data, index is out of bounds");
  return m_packets[index].getApplicationDataBytes();
}

uint16_t CCSDS::Manager::getTotalPackets() const {
  return m_packets.size();
}

std::vector<CCSDS::Packet> CCSDS::Manager::getPackets() {
  return m_packets;
}

bool CCSDS::Manager::isValidPacket(Packet packet) const {

  // prepare testData
  auto&& testPacket = packet;
  testPacket.setUpdatePacketEnable(false);
  auto testPrimaryHeader = testPacket.getPrimaryHeader();

  // auto coherence checks
  auto dataFieldBytes = testPacket.getFullDataFieldBytes();
  auto dataFieldBytesSize = dataFieldBytes.size();

  // test CRC therefore full data field coherence
  if (crc16(dataFieldBytes) != testPacket.getCRC()) {
    return false;
  }
  if ( testPrimaryHeader.getDataLength() != dataFieldBytesSize ) {
    return false;
  }
  if (testPrimaryHeader.getDataFieldHeaderFlag() != testPacket.getDataFieldHeaderFlag()) {
    return false;
  }

  // packet identification and version checks
  auto validPacket = m_packetTemplate;
  validPacket.setUpdatePacketEnable(false);
  auto validPrimaryHeader = validPacket.getPrimaryHeader();

  uint16_t validIdentificationAndVersion = (static_cast<uint16_t>(validPrimaryHeader.getVersionNumber()) << 13) |
    (validPrimaryHeader.getType() << 12) | static_cast<uint16_t>((validPrimaryHeader.getDataFieldHeaderFlag()) << 11) |
      validPrimaryHeader.getAPID();

  uint16_t testIdentificationAndVersion = (static_cast<uint16_t>(testPrimaryHeader.getVersionNumber()) << 13) |
    (testPrimaryHeader.getType() << 12) | static_cast<uint16_t>((testPrimaryHeader.getDataFieldHeaderFlag()) << 11) |
      testPrimaryHeader.getAPID();

  if (validIdentificationAndVersion != testIdentificationAndVersion) {
    return false;
  }

  if (testPrimaryHeader.getDataFieldHeaderFlag() != validPrimaryHeader.getDataFieldHeaderFlag()) {
    return false;
  }
  //todo the secondary header needs to be validated same goes for the sequence control. info can be retrieved from template?

  return true;
}


