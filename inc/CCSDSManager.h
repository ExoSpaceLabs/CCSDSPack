
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
    ResultBool setDatFieldSize( uint16_t size);
    ResultBool setApplicationData(const std::vector<uint8_t>& data);
    void setAutoUpdateEnable(bool enable);


    ResultBuffer getPacketTemplate();
    ResultBuffer getPacketAtIndex(uint16_t index);
    [[nodiscard]] ResultBuffer getApplicationData() const;
    ResultBuffer getApplicationDataAtIndex(uint16_t index);

    [[nodiscard]] Result< uint16_t > getTotalPackets() const;
    [[nodiscard]] bool getAutoUpdateEnable() const { return m_updateEnable;}
    Packet getTemplate() { return m_packetTemplate; };
    Result<std::vector<Packet>> getPackets();

    private:
    Packet m_packetTemplate{};
    bool m_updateEnable{true};  // by default every packet is set to true.
    std::vector<Packet> m_packets;
  };

} // CCSDS

#endif //CCSDSMANAGER_H
