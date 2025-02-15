#ifndef CCSDSPACKET_H
#define CCSDSPACKET_H

#include <cstdint>
#include <vector>
#include "CCSDSHeader.h"
#include "CCSDSData.h"

/**
 * @namespace CCSDS
 * @brief Contains definitions and classes for handling CCSDS headers.
 */
namespace CCSDS {

    /**
     * @brief Represents a CCSDS (Consultative Committee for Space Data Systems) packet.
     *
     * This class provides functionality to construct and manage a CCSDS packet, which
     * includes both the primary header and the data field. It allows setting and getting
     * the primary header, data field headers (PusA, PusB, PusC), and application data.
     * The packet also includes a CRC-16 checksum for error detection.
     *
     * The class provides methods for managing the packet's data structure, including
     * printing the headers and data field, calculating the CRC-16, and combining the
     * primary header, data field, and CRC into a complete packet. The header is updated
     * automatically when necessary, and it provides both raw and vector representations
     * of the data for further use or transmission.
     *
     * The `Packet` class also handles the internal state for CRC calculation and header
     * updates to ensure data consistency.
     */
    class Packet {
    public:
        Packet() = default;

        explicit Packet(                                         const std::vector<uint8_t>& data );
        explicit Packet(           const std::vector<uint8_t>& data, ESecondaryHeaderType PusType );
        explicit Packet(           const std::vector<uint8_t>& data, uint16_t headerDataSizeBytes );
        explicit Packet( const std::vector<uint8_t>& headerData, const std::vector<uint8_t> &data );

        // setters
        void setPrimaryHeader(                                                      uint64_t data );
        void setPrimaryHeader(                                   const std::vector<uint8_t>& data );
        void setPrimaryHeader(                                                 PrimaryHeader data );
        void setDataFieldHeader(                                               const PusA& header );
        void setDataFieldHeader(                                               const PusB& header );
        void setDataFieldHeader(                                               const PusC& header );
        void setDataFieldHeader(      const std::vector<uint8_t>& data, ESecondaryHeaderType type );
        void setDataFieldHeader( const uint8_t* pData, size_t sizeData, ESecondaryHeaderType type );
        void setDataFieldHeader(                                 const std::vector<uint8_t>& data );
        void setDataFieldHeader(                            const uint8_t* pData, size_t sizeData );
        void setApplicationData(                                 const std::vector<uint8_t>& data );
        void setApplicationData(                            const uint8_t* pData, size_t sizeData );

        void setSequenceFlags(                                                ESequenceFlag flags );
        void setSequenceCount(                                                     uint16_t count );
        void setDataFieldSize(                                                      uint16_t size );

        // getters
        uint64_t getPrimaryHeader64bit();
        std::vector<uint8_t> getPrimaryHeader();
        std::vector<uint8_t> getDataFieldHeader()              { return m_dataField.getDataFieldHeader(); }
        std::vector<uint8_t> getApplicationData()              { return m_dataField.getApplicationData(); }
        std::vector<uint8_t> getFullDataField()                { return   m_dataField.getFullDataField(); }
        std::vector<uint8_t> getCRCVector();
        uint16_t getCRC();
        uint16_t getDataFieldMaximumSize()    const { return m_dataField.getDataFieldAvailableSizeByes(); }
        bool getDataFieldHeaderFlag()         const {    return m_primaryHeader.getDataFieldHeaderFlag(); }


        std::vector<uint8_t> serialize();
        void deserialize(                                         const std::vector<uint8_t>& data );
        void deserialize(           const std::vector<uint8_t>& data, ESecondaryHeaderType PusType );
        void deserialize(           const std::vector<uint8_t>& data, uint16_t headerDataSizeBytes );
        void deserialize( const std::vector<uint8_t>& headerData, const std::vector<uint8_t> &data );

        uint16_t getFullPacketLength() const;

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
#endif // CCSDSPACKET_H

