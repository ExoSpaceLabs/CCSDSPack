
#ifndef CCSDSMANAGER_H
#define CCSDSMANAGER_H

#include <utility>

#include "CCSDSPacket.h"

namespace CCSDS {

  class Manager {

    public:
      Manager();
      explicit Manager(Packet packet) : m_packetTemplate(std::move(packet)) {};

      void setPacketTemplate(Packet packet);
      void setDatFieldSize(const uint16_t size) { m_packetTemplate.setDataFieldSize(size); }
      void setData(std::vector<uint8_t> data);

      void printTemplatePacket();

    // todo Make getters and setters for this class...
    // Todo Make getPacketAt(index)
    // Todo Make getPackets(); returns all packets
    private:
      Packet m_packetTemplate{};
      std::vector<uint8_t> m_Data{};
  };

} // CCSDS

#endif //CCSDSMANAGER_H
