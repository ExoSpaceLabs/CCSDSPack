#ifndef CCSDSDATA_H
#define CCSDSDATA_H

#include <string>
#include <cstdint>
#include <vector>


namespace CCSDS {
    enum SecondaryHeaderType {
        NA,
        PUS_A,
        PUS_B,
        PUS_C,
        OTHER
    };

    class DataField {
    public:
        DataField() = default;
        virtual ~DataField() = default;

        void setData(                  const uint8_t* pData, size_t sizeData );
        void setData(                       const std::vector<uint8_t>& data ) {            m_data =            data; }
        void setSecondaryHeader(       const uint8_t* pData, size_t sizeData );
        void setSecondaryHeader( const std::vector<uint8_t>& secondaryHeader ) { m_secondaryHeader = secondaryHeader; }

        void setDataPacketSize(                         const uint16_t value ) {  m_dataPacketSize =           value; }

        std::vector<uint8_t> getFullDataField();

        void printData();

    private:
        SecondaryHeaderType   m_secondaryHeaderType{};
        std::vector<uint8_t>      m_secondaryHeader{};
        std::vector<uint8_t>                 m_data{};
        uint16_t              m_dataPacketSize = 2024;
    };
}
#endif // CCSDSDATA_H

