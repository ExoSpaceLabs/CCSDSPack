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
        void setPrimaryHeader(                           const uint64_t data ) {                  m_primaryHeader.setData( data ); }
        void setPrimaryHeader(                      const PrimaryHeader data ) {                  m_primaryHeader.setData( data ); }
        void setDataField(                  const std::vector<uint8_t>& data ) {                      m_dataField.setData( data ); }
        void setDataField(       const uint8_t* pData, const size_t sizeData ) {            m_dataField.setData( pData,sizeData ); }
        void setSecondaryHeader(            const std::vector<uint8_t>& data ) {           m_dataField.setSecondaryHeader( data ); }
        void setSecondaryHeader( const uint8_t* pData, const size_t sizeData ) { m_dataField.setSecondaryHeader( pData,sizeData ); }

        // getters
        uint16_t getCRC() const { return m_CRC16; }

        // other
        void printPrimaryHeader() { m_primaryHeader.printHeader(); }
        void printDataField();

    private:
        void calculateCRC16();

        Header m_primaryHeader{};  // 6 bytes / 48 bits / 12 hex
        DataField m_dataField{};   // variable
        uint64_t m_CRC16{};               // Cyclic Redundancy check 16 bits
    };
}
#endif // CCSDSPACK_H

