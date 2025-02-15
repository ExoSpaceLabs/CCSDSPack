
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
    void setApplicationData(const std::vector<uint8_t>& data);
    void setAutoUpdateEnable(bool enable);


    std::vector<uint8_t> getPacketTemplate() { return m_packetTemplate.serialize(); }
    std::vector<uint8_t> getPacketAtIndex(uint16_t index);
    std::vector<uint8_t> getApplicationData() const;
    std::vector<uint8_t> getApplicationDataAtIndex(uint16_t index);

    uint16_t getTotalPackets() const { return m_packets.size();}
    bool getAutoUpdateEnable() const { return m_updateEnable;}

    void printTemplatePacket();
    void printPackets();


    // todo Make getters and setters for this class...
    // Todo Make getPacketAt(index)
    // Todo Make getPackets(); returns all packets
    private:
    Packet m_packetTemplate{};
    bool m_updateEnable{true};  // by default every packet is set to true.
    std::vector<Packet> m_packets;
  };

} // CCSDS

#endif //CCSDSMANAGER_H
