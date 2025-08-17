#ifndef CCSDS_PACKET_H
#define CCSDS_PACKET_H

/**
 * @mainpage CCSDSPack Library
 *
 * @section GitHub Main page
 * https://github.com/ExoSpaceLabs/CCSDSPack
 *
 * @section intro_sec Introduction
 * This library handles CCSDS packets management...
 *
 * @section features_sec Features
 * - Encode/Decode CCSDS packets
 * - Error handling with Result types
 * - Binary I/O support
 *
 * @section usage_sec Usage
 * See the class and function docs for usage examples.
 */


#include <CCSDSResult.h>
#include <vector>
#include "CCSDSHeader.h"
#include "CCSDSDataField.h"

/**
 * @namespace CCSDS
 * @brief Contains definitions and classes for handling CCSDS headers.
 */
namespace CCSDS {

  /** Configuration structure for CRC16 calculation parameters */
  struct CRC16Config {
    uint16_t polynomial = 0x1021;
    uint16_t initialValue = 0xFFFF;
    uint16_t finalXorValue = 0x0000;
  };


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

    /**
     * @brief Sets the primary header using the provided PrimaryHeader object.
     *
     * This function sets the primary header of the packet using a `PrimaryHeader`
     * object as the header data.
     *
     * @param data The `PrimaryHeader` object containing the header data.
     * @return none.
     */
    void setPrimaryHeader(PrimaryHeader data);

    /**
     * @brief Sets the primary header using the provided 64-bit data.
     *
     * This function sets the primary header of the packet using a 64-bit integer
     * as the header data.
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     *
     * @param data The 64-bit primary header data.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setPrimaryHeader(uint64_t data);

    /**
     * @brief Sets the primary header using the provided vector of uint8_tdata.
     *
     * This function sets the primary header of the packet using vector of 8-bit integers
     * as the header data.
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     *
     * @param data The vector of 8-bit integers primary header data.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setPrimaryHeader(const std::vector<uint8_t> &data);

    /**
     * @brief Sets the primary header using the provided header object.
     *
     * This function sets the primary header of the packet using the header object
     *
     * @param header
     * @return ResultBool.
     */
    void setPrimaryHeader(const Header &header);

    /**
     * @brief Sets the data field header using the provided SecondaryHeaderAbstract derived header.
     *
     * This function sets the data field header of the packet using a `SecondaryHeaderAbstract` derived
     * header object.
     *
     * @param header The `SecondaryHeaderAbstract` header object.
     * @return none.
     */
    void setDataFieldHeader(const std::shared_ptr<SecondaryHeaderAbstract> &header);

    /**
    * @brief Registers a new header type with the data field secondary header creation function.
    *
    * This function adds a new header type to the factory by associating the header's type string with a shared pointer to the header.
    *
    * @param header A shared pointer to a `SecondaryHeaderAbstract` object to register.
    */
    template <typename T>
    ResultBool RegisterSecondaryHeader() {
      FORWARD_RESULT( m_dataField.RegisterSecondaryHeader<T>());
      return true;
    }

    /**
     * @brief Sets the data field header for the packet using a vector of bytes.
     *
     * This method updates the data field header of the packet by providing the
     * data as a vector of bytes and specifying the PUS (Packet Utilization Standard) type if applicable.
     *
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     *
     * @param data A vector containing the data for the data field header.
     * @param headerType The PUS type that specifies the purpose of the data.
     *
     * @return ResultBool
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<uint8_t> &data, const std::string &headerType);

    /**
     * @brief Sets the data field header for the packet using a raw data pointer.
     *
     * This method updates the data field header of the packet by providing a
     * pointer to the raw data and its size, along with the PUS (Packet Utilization Standard) type if applicable.
     *
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     *
     * @param pData Pointer to the raw data for the data field header.
     * @param sizeData The size of the data pointed to by pData, in bytes.
     * @param headerType The PUS type that specifies the purpose of the data.
     *
     * @return ResultBool
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const uint8_t *pData, size_t sizeData, const std::string &headerType);

    /**
     * @brief Sets the data field header using the provided vector of bytes.
     *
     * This function sets the data field header of the packet using a vector
     * of bytes.
     *
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     *
     * @param data The vector containing the header bytes.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<uint8_t> &data);

    /**
     * @brief Sets the data field header using the provided pointer and size.
     *
     * This function sets the data field header of the packet using a pointer
     * to the header data and its size.
     *
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     *
     * @param pData A pointer to the header data.
     * @param sizeData The size of the header data.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const uint8_t *pData, size_t sizeData);

    /**
     * @brief Sets the application data for the packet.
     *
     * This function sets the application data in the data field of the packet
     * using a vector of bytes.
     *
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     *
     * @param data The vector containing the application data.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setApplicationData(const std::vector<uint8_t> &data);

    /**
     * @brief Sets the application data for the packet.
     *
     * This function sets the application data in the data field of the packet
     * using a pointer to the data and its size.
     *
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     *
     * @param pData A pointer to the application data.
     * @param sizeData The size of the application data.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setApplicationData(const uint8_t *pData, size_t sizeData);

    /**
     * @brief Sets the sequence flags for the packet's primary header.
     *
     * This method updates the sequence flags in the primary header of the packet.
     * The sequence flags indicate the position or type of the data segment within
     * the CCSDS telemetry transfer frame (e.g., first segment, last segment, etc.).
     *
     * @param flags The sequence flag to be set, represented by the ESequenceFlag enum : uint8_t.
     */
    void setSequenceFlags(ESequenceFlag flags);

    /** @brief Sets the sequence count for the packet. */
    [[nodiscard]] ResultBool setSequenceCount(uint16_t count);

    /**
     * @brief Sets the maximum data packet size for the CCSDS DataField.
     *
     * This method updates the maximum allowed size for the data packet.
     * The data packet size is used to validate that the combined size of
     * the header and application data does not exceed this limit.
     *
     * @param size The maximum size of the data packet, in bytes.
     */
    void setDataFieldSize(uint16_t size);

    /**
     * needs to be called as soon as possible, probably also from constructor.
     * @param enable
     */
    void setUpdatePacketEnable(bool enable);

    /** @brief Deserializes a vector of bytes into a CCSDS packet. */
    [[nodiscard]] ResultBool deserialize(const std::vector<uint8_t> &data);

    /** @brief Deserializes a CCSDS packet using a vector and a registered header type. */
    [[nodiscard]] ResultBool deserialize(const std::vector<uint8_t> &data, const std::string& headerType, int headerSize = -1);

    /** @brief Deserializes a CCSDS packet using a vector and a header data size. */
    [[nodiscard]] ResultBool deserialize(const std::vector<uint8_t> &data, uint16_t headerDataSizeBytes);

    /** @brief Deserializes a CCSDS packet using separate header and data vectors. */
    [[nodiscard]] ResultBool deserialize(const std::vector<uint8_t> &headerData, const std::vector<uint8_t> &data);

    /**
     * @brief Retrieves the primary header of the packet.
     *
     * This function ensures that the primary header is updated by calling the
     * `updatePrimaryHeader()` function, and then returns the full 48-bit primary
     * header as a 64-bit integer.
     *
     * @return The 64-bit primary header of the packet.
     */
    uint64_t getPrimaryHeader64bit();

    /** @brief Retrieves the current size of the CCSDS Packet
     *
     * @return 16bit unsigned integer
     */
    uint16_t getFullPacketLength();

    /**
     * @brief Retrieves the full packet as a vector of bytes.
     *
     * Combines the primary header, data field, and CRC-16 checksum into
     * a single vector. Ensures that the header and data field sizes meet
     * minimum requirements.
     *
     * @note Header size must be 6 bytes and data field size must be greater than 1 byte.
     *
     * @return A vector containing the full packet in byte form.
     */
    std::vector<uint8_t> serialize();

    /**
     * @brief Retrieves the primary header of the packet as a vector of bytes.
     *
     * Extracts the 48-bit primary header and converts it into a six-element
     * vector, ordered from the most significant byte (MSB) to the least
     * significant byte (LSB).
     *
     * @return A vector containing the six bytes of the primary header.
     */
    std::vector<uint8_t> getPrimaryHeaderBytes();

    /**
     * @brief Retrieves the secondary header data from the data field.
     *
     * @return A vector containing the Secondary header data.
     */
    std::vector<uint8_t> getDataFieldHeaderBytes();

    /**
     * @brief Retrieves the application data from the data field.
     *
     * @return A vector containing the Application data.
     */
    std::vector<uint8_t> getApplicationDataBytes();

    /**
     * @brief Retrieves the full data field data. i.e. Application data and Secondary header data if applicable.
     *
     * @return A vector containing the data field data.
     */
    std::vector<uint8_t> getFullDataFieldBytes();

    /**
     * @brief Retrieves the CRC-16 checksum as a vector of bytes.
     *
     * The checksum is split into its most significant byte (MSB) and
     * least significant byte (LSB) and stored in a two-element vector.
     *
     * @return A vector containing the MSB and LSB of the CRC-16 checksum.
     */
    std::vector<uint8_t> getCRCVectorBytes();

    /**
     * @brief Computes and retrieves the CRC-16 checksum of the packet.
     *
     * If the CRC-16 has not already been calculated, this function computes it
     * using the full data field of the packet. The result is cached for
     * future calls to improve performance.
     *
     * @return The 16-bit CRC-16 checksum.
     */
    uint16_t getCRC();

    /** @brief returns the maximum data field size */
    uint16_t getDataFieldMaximumSize() const;

    /** @ returns the data field header flag */
    bool getDataFieldHeaderFlag();

    /** @brief returns the CCSDS packet's DataField. */
    DataField &getDataField();

    /** @brief returns the CCSDS packet's Primary Header. */
    Header &getPrimaryHeader();

    /**
     * @brief Sets the crc configuration of the crc calculation
     *
     * This method sets the following variables to be used when performing the
     * crc calculation of data passed as a structure of CRC16Config.
     *  - polynomial
     *  - initial value
     *  - final xor value
     *
     * @param crcConfig
     */
    void setCrcConfig(const CRC16Config crcConfig) {m_CRC16Config = crcConfig;}


    /**
     * @brief Sets the crc configuration of the crc calculation
     *
     * This method sets the following variables to be used when performing the
     * crc calculation of data passed as a structure of CRC16Config.
     *  - polynomial
     *  - initial value
     *  - final xor value
     *
     * @param polynomial
     * @param initialValue
     * @param finalXorValue
     */
    void setCrcConfig(const uint16_t polynomial, const uint16_t initialValue, const uint16_t finalXorValue) {m_CRC16Config = {polynomial, initialValue, finalXorValue};}

    /**
     * @brief Updates Primary headers data field size.
     *
     * Uses the currently set data field size to set the header.
     *
     * @return none.
     */
    void update();

    /**
     * @brief Loads a packet from a configuration file, including secondary header if present.
     *
     * @param configPath path to the configuration file.
     * @return ResultBool
     */
    ResultBool loadFromConfigFile(const std::string &configPath);

    /**
     * @brief Loads a packet from a configuration object, including secondary header if present.
     *
     * @param cfg configuration object.
     * @return ResultBool
     */
    ResultBool loadFromConfig(const Config &cfg);


  private:
    Header m_primaryHeader{};        ///< 6 bytes / 48 bits / 12 hex
    DataField m_dataField{};         ///< variable
    uint16_t m_CRC16{};              ///< Cyclic Redundancy check 16 bits
    CRC16Config m_CRC16Config;       ///< structure holding configuration of crc calculation.
    bool m_updateStatus{false};      ///< When setting data thus value should be set to false.
    bool m_enableUpdatePacket{true}; ///< Enables primary header and secondary header update.
    uint16_t m_sequenceCounter{0};
  };
}
#endif // CCSDS_PACKET_H
