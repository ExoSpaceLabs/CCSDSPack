#ifndef CCSDSPACKET_H
#define CCSDSPACKET_H

#include <CCSDSResult.h>
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

        // setters
        void setPrimaryHeader(                                   PrimaryHeader data );
        [[nodiscard]] ResultBool setPrimaryHeader(                   uint64_t  data );
        [[nodiscard]] ResultBool setPrimaryHeader( const std::vector<uint8_t> &data );

        void setDataFieldHeader(                                                       const PusA& header );
        void setDataFieldHeader(                                                       const PusB& header );
        void setDataFieldHeader(                                                       const PusC& header );

        [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<uint8_t> &data, ESecondaryHeaderType type);

        [[nodiscard]] ResultBool setDataFieldHeader(const uint8_t *pData, size_t sizeData, ESecondaryHeaderType type);

        [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<uint8_t> &data);

        [[nodiscard]] ResultBool setDataFieldHeader(const uint8_t *pData, size_t sizeData);

        [[nodiscard]] ResultBool setApplicationData(const std::vector<uint8_t> &data);

        [[nodiscard]] ResultBool setApplicationData(const uint8_t *pData, size_t sizeData);

        void setSequenceFlags(                                                        ESequenceFlag flags );
        void setSequenceCount(                                                             uint16_t count );
        void setDataFieldSize(                                                              uint16_t size );
        void setUpdatePacketEnable(                                                           bool enable );

        [[nodiscard]] ResultBool deserialize(                                         const std::vector<uint8_t> &data );
        [[nodiscard]] ResultBool deserialize(           const std::vector<uint8_t> &data, ESecondaryHeaderType PusType );
        [[nodiscard]] ResultBool deserialize(           const std::vector<uint8_t> &data, uint16_t headerDataSizeBytes );
        [[nodiscard]] ResultBool deserialize( const std::vector<uint8_t> &headerData, const std::vector<uint8_t> &data );

        // getters
        uint64_t getPrimaryHeader64bit();
        uint16_t getFullPacketLength();
        std::vector<uint8_t> serialize();
        std::vector<uint8_t> getPrimaryHeader();
        std::vector<uint8_t> getDataFieldHeader();
        std::vector<uint8_t> getApplicationData();
        std::vector<uint8_t> getFullDataField();
        std::vector<uint8_t> getCRCVector();
        uint16_t getCRC();
        uint16_t getDataFieldMaximumSize();
        bool getDataFieldHeaderFlag();
        Result<CCSDS::DataField> getDataField();

        // other


        void update();
    private:

        Header            m_primaryHeader{};      // 6 bytes / 48 bits / 12 hex
        DataField             m_dataField{};      // variable
        uint16_t                  m_CRC16{};      // Cyclic Redundancy check 16 bits

        bool        m_updateStatus{ false };      // When setting data thus value should be set to false.
        bool   m_enableUpdatePacket{ true };      // Enables primary header and secondary header update.
        uint16_t     m_sequenceCounter{ 0 };
    };
}
#endif // CCSDSPACKET_H

