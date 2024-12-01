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
        void setDataPacketSize(                         const uint16_t value ) {      m_dataPacketSize =           value; }
        void setData(                       const std::vector<uint8_t>& data ) {                m_data =            data; }
        void setSecondaryHeader( const std::vector<uint8_t>& secondaryHeader ) {     m_secondaryHeader = secondaryHeader; }


        std::vector<uint8_t> getFullDataField();

        void printData();

    private:
        SecondaryHeaderType m_secondaryHeaderType{};
        std::vector<uint8_t>  m_secondaryHeader{};
        std::vector<uint8_t> m_data{};
        uint16_t m_dataPacketSize = 2024; // excluding header
    };
}
#endif // CCSDSDATA_H

