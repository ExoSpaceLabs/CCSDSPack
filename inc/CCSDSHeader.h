#ifndef CCSDS_HEADER_H
#define CCSDS_HEADER_H

#include <CCSDSResult.h>
#include <cstdint>
#include <vector>

namespace CCSDS {
  /**
   * @brief Represents the sequence flags used in CCSDS telemetry transfer frames.
   *
   * This enum defines the possible sequence flag values that indicate the type
   * and position of a data segment in a telemetry frame:
   * - CONTINUING_SEGMENT: An intermediate segment of a packet.
   * - FIRST_SEGMENT: The first segment of a new packet.
   * - LAST_SEGMENT: The last segment of a packet that spans multiple frames.
   * - UNSEGMENTED: A complete unsegmented packet contained in a single frame.
   */
  enum ESequenceFlag : std::uint8_t {
    CONTINUING_SEGMENT, ///< 00 Intermediate segment of a packet.
    FIRST_SEGMENT,      ///< 01 First segment of a new packet.
    LAST_SEGMENT,       ///< 10 Last segment of a multi-frame packet.
    UNSEGMENTED         ///< 11 Complete packet in a single frame.
  };

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
    std::uint8_t versionNumber{};              ///< 3 bit first of packet identification
    std::uint8_t type{};                       ///< 1 bit second of packet identification
    std::uint8_t dataFieldHeaderFlag{};        ///< 1 bit third of packet identification
    std::uint16_t APID{};                      ///< 11 bit last of packet identification

    //packet sequence control 16 bit 4 hex
    std::uint8_t sequenceFlags{};              ///< 2 bit first of sequence control
    std::uint16_t sequenceCount{};             ///< 14 bit last of sequence control

    // data packet length
    std::uint16_t dataLength{};                ///< 16 bits

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
    PrimaryHeader(const std::uint8_t versionNumber_value, const std::uint8_t type_value, const std::uint8_t dataFieldHeaderFlag_value,
                  const std::uint16_t APID_value, const std::uint8_t sequenceFlag_value, const std::uint16_t sequenceCount_value,
                  const std::uint16_t dataLength_value) : versionNumber(versionNumber_value), type(type_value),
                                                     dataFieldHeaderFlag(dataFieldHeaderFlag_value),
                                                     APID(APID_value), sequenceFlags(sequenceFlag_value),
                                                     sequenceCount(sequenceCount_value),
                                                     dataLength(dataLength_value) {
    }
  };

  /**
   * @class Header
   * @brief Manages the decomposition and manipulation of CCSDS primary headers.
   */
  class Header {
  public:
    Header() = default;

    [[nodiscard]] std::uint8_t getVersionNumber()       const { return m_versionNumber;       } ///< 3 bits
    [[nodiscard]] std::uint8_t getType()                const { return m_type;                } ///< 1 bits
    [[nodiscard]] std::uint8_t getDataFieldHeaderFlag() const { return m_dataFieldHeaderFlag; } ///< 1 bits
    [[nodiscard]] std::uint16_t getAPID()               const { return m_APID;                } ///< 11 bits
    [[nodiscard]] std::uint8_t getSequenceFlags()       const { return m_sequenceFlags;       } ///< 2 bits
    [[nodiscard]] std::uint16_t getSequenceCount()      const { return m_sequenceCount;       } ///< 14 bits
    [[nodiscard]] std::uint16_t getDataLength()         const { return m_dataLength;          } ///< 16 bits

    /**
     * @brief decomposes the Primary header class and returns it as a vector of bytes.
     * @note if data has not been set it is initialized as all 0s.
     *
     * @return std::vector<std::uint8_t>
     */
    std::vector<std::uint8_t> serialize();

    /**
     * @brief Computes and retrieves the full header as a 64-bit value. Combines individual header fields into a
     * single 64-bit representation.
     *
     * @note if data has not been set it is initialized as all 0s.
     *
     * @return The full header as a 64-bit integer.
     */
    std::uint64_t getFullHeader() {
      m_packetSequenceControl = (m_sequenceFlags << 14) | m_sequenceCount;
      m_packetIdentificationAndVersion = (m_versionNumber << 13) | (m_type << 12) | (m_dataFieldHeaderFlag << 11) |
                                         m_APID;
      return (static_cast<std::uint64_t>(m_packetIdentificationAndVersion) << 32) | (
               static_cast<std::uint32_t>(m_packetSequenceControl) << 16) | m_dataLength;
    }

    void setVersionNumber      ( const  std::uint8_t &value ) {       m_versionNumber = value & 0x0007; } ///< 3 bits
    void setType               ( const  std::uint8_t &value ) {                m_type = value & 0x0001; } ///< 1 bits
    void setDataFieldHeaderFlag( const  std::uint8_t &value ) { m_dataFieldHeaderFlag = value & 0x0001; } ///< 1 bits
    void setAPID               ( const std::uint16_t &value ) {                m_APID = value & 0x07FF; } ///< 11 bits
    void setSequenceFlags      ( const  std::uint8_t &value ) {       m_sequenceFlags = value & 0x0003; } ///< 2 bits
    void setSequenceCount      ( const std::uint16_t &value ) {       m_sequenceCount = value & 0x3FFF; } ///< 14 bits
    void setDataLength         ( const std::uint16_t &value ) {          m_dataLength = value;          } ///< 16 bits

    /**
     * @brief Sets the header data from a 64-bit integer representation.
     *
     * Decomposes the 64-bit input data into various header fields, including the version number,
     * type, data field header flag, APID, sequence flags, sequence count, and data length.
     *
     * @note returns an error code if the provided data exceeds maximum available value (max 6 bytes).
     *
     * @param data The 64-bit integer representing the header data.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setData(const std::uint64_t &data);

    /**
     * @brief Sets the header data from a 64-bit integer representation.
     *
     * Decomposes the 64-bit input data into various header fields, including the version number,
     * type, data field header flag, APID, sequence flags, sequence count, and data length.
     *
     * @note returns an error code if the provided data exceeds maximum available value (max 6 bytes i.e. elements).
     *
     * @param data reference to an std::uint8_t vector.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data);


    /**
     * @brief Sets the header data from a `PrimaryHeader` structure.
     *
     * Assigns values from a `PrimaryHeader` structure to the internal header fields.
     * Combines certain fields into their packed representations for efficient storage.
     *
     * @param data A `PrimaryHeader` structure containing the header data.
     * @return none.
     */
    void setData(const PrimaryHeader &data);

  private:
    // version and packet identification 16 bit 4 hex
    std::uint8_t m_versionNumber{};                    ///< 3 bit first of packet identification

    // packet identification 4 hex
    std::uint8_t m_type{};                             ///< 1 bit second of packet identification
    std::uint8_t m_dataFieldHeaderFlag{};              ///< 1 bit third of packet identification
    std::uint16_t m_APID{};                            ///< 11 bit last of packet identification

    // packet sequence control 16 bit 4 hex
    std::uint8_t m_sequenceFlags{UNSEGMENTED};         ///< 2 bit first of sequence control / ESequenceFlag enum.
    std::uint16_t m_sequenceCount{};                   ///< 14 bit last of sequence control

    // full packet size 48 bit fixed 6 byes
    std::uint16_t m_packetIdentificationAndVersion{};  ///< packet id and version 16 bit 4 hex
    std::uint16_t m_packetSequenceControl{};           ///< packet sequence control 16 bit 4 hex
    std::uint16_t m_dataLength{};                      ///< data packet length 16 bits 4 hex
  };
}
#endif // CCSDS_HEADER_H
