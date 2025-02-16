
#ifndef CCSDSMANAGER_H
#define CCSDSMANAGER_H

#include <utility>

#include "CCSDSPacket.h"
#include "CCSDSResult.h"

namespace CCSDS {

  class Manager {

    public:
    Manager();
    explicit Manager(Packet packet) : m_packetTemplate(std::move(packet)) {};

    void setPacketTemplate(Packet packet);
    void setDatFieldSize(const uint16_t size) { m_packetTemplate.setDataFieldSize(size); }
    void setApplicationData(const std::vector<uint8_t>& data);
    void setAutoUpdateEnable(bool enable);


    Result<std::vector<uint8_t>> getPacketTemplate();
    Result<std::vector<uint8_t>> getPacketAtIndex(uint16_t index);
    [[nodiscard]] Result<std::vector<uint8_t>> getApplicationData() const;
    Result<std::vector<uint8_t>> getApplicationDataAtIndex(uint16_t index);

    [[nodiscard]] Result< uint16_t> getTotalPackets() const;
    [[nodiscard]] bool getAutoUpdateEnable() const { return m_updateEnable;}
    Packet getTemplate() { return m_packetTemplate; };
    Result<std::vector<Packet>> getPackets();

    void printTemplatePacket();

    private:
    Packet m_packetTemplate{};
    bool m_updateEnable{true};  // by default every packet is set to true.
    std::vector<Packet> m_packets;
  };

} // CCSDS

#endif //CCSDSMANAGER_H
