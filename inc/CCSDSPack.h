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
        void setPrimaryHeader(                           const uint64_t data ) {                  m_primaryHeader.setData( data ); m_crcCalculated = false; }
        void setPrimaryHeader(                      const PrimaryHeader data ) {                  m_primaryHeader.setData( data ); m_crcCalculated = false; }
        void setDataFieldHeader(                           const PusA header ) {         m_dataField.setDataFieldHeader( header ); m_crcCalculated = false; }
        void setDataFieldHeader(                           const PusB header ) {         m_dataField.setDataFieldHeader( header ); m_crcCalculated = false; }
        void setDataFieldHeader(                          const PusC &header ) {      m_dataField.setDataFieldHeader( header ); m_crcCalculated = false; }
        void setDataFieldHeader(            const std::vector<uint8_t>& data ) {           m_dataField.setDataFieldHeader( data ); m_crcCalculated = false; }
        void setDataFieldHeader( const uint8_t* pData, const size_t sizeData ) { m_dataField.setDataFieldHeader( pData,sizeData ); m_crcCalculated = false; }
        void setApplicationData(            const std::vector<uint8_t>& data ) {           m_dataField.setApplicationData( data ); m_crcCalculated = false; }
        void setApplicationData( const uint8_t* pData, const size_t sizeData ) { m_dataField.setApplicationData( pData,sizeData ); m_crcCalculated = false; }

        // getters
        uint64_t getPrimaryHeader()               { return  m_primaryHeader.getFullHeader(); };
        std::vector<uint8_t> getDataFieldHeader() { return m_dataField.getDataFieldHeader(); };
        std::vector<uint8_t> getApplicationData() { return m_dataField.getApplicationData(); };
        std::vector<uint8_t> getFullDataField()   { return   m_dataField.getFullDataField(); };
        std::vector<uint8_t> getPrimaryHeaderVector();
        std::vector<uint8_t> getFullPacket();
        std::vector<uint8_t> getCRCVector();
        uint16_t getCRC();

        // other
        void printPrimaryHeader() { m_primaryHeader.printHeader(); }
        void printDataField();

    private:

        Header m_primaryHeader{};         // 6 bytes / 48 bits / 12 hex
        DataField m_dataField{};          // variable
        uint16_t m_CRC16{};               // Cyclic Redundancy check 16 bits

        bool m_crcCalculated{false};      // When setting data thus value should be set to false.
    };
}
#endif // CCSDSPACK_H

