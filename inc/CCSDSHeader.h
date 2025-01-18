#ifndef CCSDSHEADER_H
#define CCSDSHEADER_H

#include <cstdint>

/**
* PrimaryHeader Structure
*
* parameters:
* uint8_t versionNumber_value,       // 3 bit
* uint8_t type_value,                // 1 bit
* uint8_t dataFieldHeaderFlag_value, // 1 bit
* uint16_t APID_value,               // 11 bit
* #uint8_t sequenceFlag_value,       // 2 bit
* uint16_t sequenceCount_value,      // 14 bit
* uint16_t dataLength_value          // 16 bit
*
*/

/**
 * @namespace CCSDS
 * @brief Contains definitions and classes for handling CCSDS headers.
 */
namespace CCSDS {
    /**
     * @struct PrimaryHeader
     * @brief Represents the primary header of a CCSDS packet.
     *
     * This structure is used to encapsulate the key fields of the CCSDS primary header.
     *
     * Fields:
     * - versionNumber: 3-bit field indicating the version of the CCSDS standard.
     * - type: 1-bit field indicating the type of the packet (telemetry or telecommand).
     * - dataFieldHeaderFlag: 1-bit field indicating if a secondary header is present.
     * - APID: 11-bit field identifying the application process.
     * - sequenceFlags: 2-bit field indicating the sequence type of the packet.
     * - sequenceCount: 14-bit field indicating the sequence number of the packet.
     * - dataLength: 16-bit field specifying the data length of the packet payload.
     */
    struct PrimaryHeader {
        // version and packet identification 16 bit 4 hex
        uint8_t versionNumber;         // 3 bit

        // packet identification 4 hex
        uint8_t type;                  // 1 bit
        uint8_t dataFieldHeaderFlag;   // 1 bit
        uint16_t APID;                 // 11 bit

        //packet sequence control 16 bit 4 hex
        uint8_t sequenceFlags;         // 2 bit
        uint16_t sequenceCount;        // 14 bit

        // data packet length
        uint16_t dataLength;           // 16 bits

        /**
         * @brief Constructs a PrimaryHeader with specified field values.
         *
         * @param versionNumber_value 3-bit version number.
         * @param type_value 1-bit type value (0 or 1).
         * @param dataFieldHeaderFlag_value 1-bit data field header flag.
         * @param APID_value 11-bit application process identifier.
         * @param sequenceFlag_value 2-bit sequence flag.
         * @param sequenceCount_value 14-bit sequence count.
         * @param dataLength_value 16-bit data length.
         */
        PrimaryHeader(uint8_t versionNumber_value, uint8_t type_value, uint8_t dataFieldHeaderFlag_value, uint16_t APID_value,
                     uint8_t sequenceFlag_value, uint16_t sequenceCount_value,  uint16_t dataLength_value) :

                      versionNumber(versionNumber_value), type(type_value),  dataFieldHeaderFlag(dataFieldHeaderFlag_value),
                      APID(APID_value), sequenceFlags(sequenceFlag_value), sequenceCount(sequenceCount_value),
                      dataLength(dataLength_value) {}
    };
    /**
     * @class Header
     * @brief Manages the decomposition and manipulation of CCSDS primary headers.
     */
    class Header {
    public:
        Header() = default;

        /**
        * @brief Constructs a Header from a 64-bit data representation.
        * @param data The 64-bit integer representing the header fields.
        */
        explicit Header(const uint64_t data)     { setData(data);                }

        // getters
        uint8_t  getVersionNumber() const        { return m_versionNumber;       }
        uint8_t  getType() const                 { return m_type;                }
        uint8_t  getDataFieldHeaderFlag() const  { return m_dataFieldHeaderFlag; }
        uint16_t getAPID() const                 { return m_APID;                }
        uint8_t  getSequenceFlags() const        { return m_sequenceFlags;       }
        uint16_t getSequenceCount() const        { return m_sequenceCount;       }
        uint16_t getDataLength() const           { return m_dataLength;          }

        /**
         * @brief Computes and retrieves the full header as a 64-bit value.
         *
         * Combines individual header fields into a single 64-bit representation.
         *
         * @return The full header as a 64-bit integer.
         */
        uint64_t getFullHeader(){
            m_packetSequenceControl = (m_sequenceFlags << 14) | m_sequenceCount;
            m_packetIdentificationAndVersion = (m_versionNumber << 13) | (m_type << 12) | (m_dataFieldHeaderFlag << 11) | m_APID;
            return (static_cast<uint64_t>(m_packetIdentificationAndVersion) << 32) | (static_cast<uint32_t>(m_packetSequenceControl) << 16) | m_dataLength;
        }


        // setters
        void setVersionNumber(const uint8_t value)        { m_versionNumber       = value & 0x0007; }
        void setType(const uint8_t value)                 { m_type                = value & 0x0001; }
        void setDataFieldheaderFlag(const uint8_t value)  { m_dataFieldHeaderFlag = value & 0x0001; }
        void setAPID(const uint16_t value)                { m_APID                = value & 0x07FF; }
        void setSequenceFlags(const uint8_t value)        { m_sequenceFlags       = value & 0x0003; }
        void setSequenceCount(const uint16_t value)       { m_sequenceCount       = value & 0x3FFF; }
        void setDataLength(const uint16_t value)          { m_dataLength          = value;          }


        // Full data setter
        void setData(uint64_t data);

        // Full data setter
        void setData(PrimaryHeader data);

        // print out the header
        void printHeader();



    private:
        // full packet size 48 bit fixed 6 byes
        uint16_t m_packetIdentificationAndVersion{};

        // version and packet identification 16 bit 4 hex
        uint8_t m_versionNumber{};            // 3 bit

        // packet identification 4 hex
        uint8_t m_type{};                     // 1 bit
        uint8_t m_dataFieldHeaderFlag{};      // 1 bit
        uint16_t m_APID{};                    // 11 bit

        //packet sequence control 16 bit 4 hex
        uint16_t m_packetSequenceControl{};
        uint8_t m_sequenceFlags{};            // 2 bit
        uint16_t m_sequenceCount{};           // 14 bit

        // data packet length
        uint16_t m_dataLength{};              // 16 bits
    };
}
#endif // CCSDSHEADER_H