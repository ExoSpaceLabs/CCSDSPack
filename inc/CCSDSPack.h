#ifndef CCSDSPACK_H
#define CCSDSPACK_H

#include <cstdint>
#include <vector>
#include "CCSDSHeader.h"
#include "CCSDSData.h"

namespace CCSDS {
    class Packet {
    public:
        Packet() = default;

        // setters
        void setPrimaryHeader(                           uint64_t data );
        void setPrimaryHeader(                      PrimaryHeader data );
        void setDataFieldHeader(                    const PusA& header );
        void setDataFieldHeader(                    const PusB& header );
        void setDataFieldHeader(                    const PusC& header );
        void setDataFieldHeader(      const std::vector<uint8_t>& data );
        void setDataFieldHeader( const uint8_t* pData, size_t sizeData );
        void setApplicationData(      const std::vector<uint8_t>& data );
        void setApplicationData( const uint8_t* pData, size_t sizeData );

        // getters
        uint64_t getPrimaryHeader();
        std::vector<uint8_t> getPrimaryHeaderVector();
        std::vector<uint8_t> getDataFieldHeader()      { return m_dataField.getDataFieldHeader(); }
        std::vector<uint8_t> getApplicationData()      { return m_dataField.getApplicationData(); }
        std::vector<uint8_t> getFullDataField()        { return   m_dataField.getFullDataField(); }
        std::vector<uint8_t> getFullPacket();
        std::vector<uint8_t> getCRCVector();
        uint16_t getCRC();

        // other
        void printPrimaryHeader() { m_primaryHeader.printHeader(); }
        void printDataField();

    private:
        void updatePrimaryHeader();

        Header m_primaryHeader{};         // 6 bytes / 48 bits / 12 hex
        DataField m_dataField{};          // variable
        uint16_t m_CRC16{};               // Cyclic Redundancy check 16 bits

        bool m_crcCalculated{false};      // When setting data thus value should be set to false.
        bool m_updatedHeader{false};      // When setting data thus value should be set to false.
        uint16_t m_sequenceCounter{0};
    };
}
#endif // CCSDSPACK_H

