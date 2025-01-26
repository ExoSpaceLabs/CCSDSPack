
#include "CCSDSManager.h"

#include <utility>
#include "CCSDSUtils.h"

void CCSDS::Manager::setPacketTemplate(CCSDS::Packet packet){
  m_packetTemplate = std::move(packet);
}

void CCSDS::Manager::printTemplatePacket() {
  m_packetTemplate.printPrimaryHeader();
  m_packetTemplate.printDataField();
}
