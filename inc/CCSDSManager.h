
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

    void setData(const std::vector<uint8_t>& data);

    std::vector<uint8_t> getPacketTemplate() { return m_packetTemplate.serialize(); }
    uint16_t getTotalPackets() const { return m_packets.size();}
    std::vector<uint8_t> getPacketAtIndex(uint16_t index);

    void printTemplatePacket();
    void printPackets();


    // todo Make getters and setters for this class...
    // Todo Make getPacketAt(index)
    // Todo Make getPackets(); returns all packets
    private:
    Packet m_packetTemplate{};
    std::vector<uint8_t> m_data{};
    std::vector<Packet> m_packets;
  };

} // CCSDS

#endif //CCSDSMANAGER_H
