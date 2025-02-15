
#include "CCSDSManager.h"

#include <utility>
#include "CCSDSUtils.h"
#include <iostream>

void CCSDS::Manager::setPacketTemplate(CCSDS::Packet packet){
  m_packetTemplate = std::move(packet);
}

void CCSDS::Manager::setApplicationData(const std::vector<uint8_t>& data) {
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
      newPacket.setApplicationData(tmp);
      newPacket.setSequenceCount(sequenceCount);
      newPacket.setSequenceFlags(sequenceFlag);
    } else {
      tmp.insert(tmp.end(), data.begin() + i, data.begin() + i + remainderBytes);
      i += remainderBytes;
      if ( sequenceFlag != UNSEGMENTED) {
        newPacket.setSequenceCount(sequenceCount);
        newPacket.setSequenceFlags(LAST_SEGMENT);
      }
      newPacket.setApplicationData(tmp);
    }
    m_packets.push_back(std::move(newPacket));
    sequenceCount++;
  }
}

void CCSDS::Manager::setAutoUpdateEnable(const bool enable) {
  m_updateEnable = enable;
  for (auto& packet : m_packets) {
    packet.setUpdatePacketEnable(enable);
  }
}

std::vector<uint8_t> CCSDS::Manager::getPacketAtIndex(const uint16_t index) {
  if (index <= m_packets.size()) {
    return m_packets[index].serialize();
  }
  return {};
}

std::vector<uint8_t> CCSDS::Manager::getApplicationData() const {
  std::vector<uint8_t> data;
  for (auto packet : m_packets) {
    auto applicationData = packet.getApplicationData();
    data.insert(data.end(),    applicationData.begin(),    applicationData.end());
  }
  return data;
}

std::vector<uint8_t> CCSDS::Manager::getApplicationDataAtIndex(const uint16_t index) {
  if (index <= m_packets.size()) {
    return m_packets[index].getApplicationData();
  }
  return {};
}

void CCSDS::Manager::printTemplatePacket() {
  m_packetTemplate.printPrimaryHeader();
  m_packetTemplate.printDataField();
}

void CCSDS::Manager::printPackets() {
  int idx = 0;
  for (auto& packet : m_packets) {
    std::cout << "[ CCSDS Manager ] Printing Packet [ "<< idx << " ]:" << std::endl;
    std::cout << "[ CCSDS Manager ] Data ";
    printBufferData(packet.serialize());

    packet.printPrimaryHeader();
    packet.printDataField();

    idx++;
  }
}
