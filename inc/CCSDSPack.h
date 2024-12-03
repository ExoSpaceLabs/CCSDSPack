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
        void setDataFieldHeader(                           const PusA header ) {         m_dataField.setDataFieldHeader( header ); }
        void setDataFieldHeader(                           const PusB header ) {         m_dataField.setDataFieldHeader( header ); }
        void setDataFieldHeader(                          const PusC &header ) {      m_dataField.setDataFieldHeader( header ); }
        void setDataFieldHeader(            const std::vector<uint8_t>& data ) {           m_dataField.setDataFieldHeader( data ); }
        void setDataFieldHeader( const uint8_t* pData, const size_t sizeData ) { m_dataField.setDataFieldHeader( pData,sizeData ); }
        void setApplicationData(            const std::vector<uint8_t>& data ) {           m_dataField.setApplicationData( data ); }
        void setApplicationData( const uint8_t* pData, const size_t sizeData ) { m_dataField.setApplicationData( pData,sizeData ); }

        uint64_t getPrimaryHeader() { return m_primaryHeader.getFullHeader(); };
        std::vector<uint8_t> getDataFieldHeader() { return m_dataField.getDataFieldHeader();};
        std::vector<uint8_t> getApplicationData() { return m_dataField.getApplicationData();};

        // getters
        uint16_t getCRC() const { return m_CRC16; }

        // other
        void calculateCRC16();
        void printPrimaryHeader() { m_primaryHeader.printHeader(); }
        void printDataField();

    private:

        Header m_primaryHeader{};  // 6 bytes / 48 bits / 12 hex
        DataField m_dataField{};   // variable
        uint16_t m_CRC16{};               // Cyclic Redundancy check 16 bits
    };
}
#endif // CCSDSPACK_H

