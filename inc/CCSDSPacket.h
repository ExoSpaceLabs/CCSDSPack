// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSPacket.h
 * @brief Defines the CCSDS 133.0-B-2 EC2 Space Packet PDU container and mission-profile CRC trailer.
 *
 * A Packet owns one primary header and one Packet Data Field. CCSDSPack may reserve
 * the final two Packet Data Field octets for a mission-profile CRC16 trailer. The
 * trailer is not a third CCSDS Space Packet structural field. Inspection APIs are
 * non-mutating. Call update() or serialize() when dependent fields such as Packet
 * Data Length, sequence count, secondary-header contents, or CRC16 must be finalized.
 */
#ifndef CCSDS_PACKET_H
#define CCSDS_PACKET_H

#include <CCSDSResult.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "CCSDSHeader.h"
#include "CCSDSDataField.h"

namespace CCSDS {
  /**
   * @struct CRC16Config
   * @brief Parameters for the optional CCSDSPack mission-profile CRC16 trailer.
   *
   * The defaults implement CRC-16/CCITT-FALSE: polynomial 0x1021, initial value
   * 0xFFFF, and no final XOR. When enabled, CCSDSPack reserves the final two
   * octets of the CCSDS Packet Data Field for the trailer. The CRC covers the
   * serialized six-byte primary header followed by all preceding Packet Data
   * Field octets, excluding the two CRC octets themselves.
   *
   * @note CCSDS 133.0-B-2 does not define this CRC trailer. It is a documented
   * CCSDSPack mission-profile convention carried inside the Packet Data Field.
   */
  struct CRC16Config {
    std::uint16_t polynomial = 0x1021;   ///< CRC generator polynomial.
    std::uint16_t initialValue = 0xFFFF; ///< Initial CRC register value.
    std::uint16_t finalXorValue = 0x0000;///< Value XORed with the final CRC.
  };

  /**
   * @enum PacketErrorControlMode
   * @brief Selects whether the Packet Data Field ends with the CCSDSPack CRC16 trailer.
   *
   * Parsing never infers the mode from trailing bytes. Configure the receiving
   * Packet before deserialization when the stream does not use the default CRC16
   * profile. The trailer octets are included in the CCSDS Packet Data Length.
   */
  enum class PacketErrorControlMode : std::uint8_t {
    None = 0, ///< No mission-profile CRC trailer is serialized or validated.
    CRC16 = 1 ///< Reserve and validate a two-byte CRC16 trailer. This is the v1 default.
  };

  /**
   * @class Packet
   * @brief Owns, serializes, and parses one CCSDS 133.0-B-2 EC2 Space Packet PDU.
   *
   * Packet provides checked primary-header assignment, optional typed or raw
   * secondary headers, application data, bounded parsing, an optional CCSDSPack
   * CRC16 trailer, and explicit finalization. It implements the Space Packet PDU
   * profile, not the complete abstract Packet Service, Octet String Service, or
   * protocol implementation conformance statement defined by CCSDS 133.0-B-2.
   * The object represents exactly one packet. Use Manager when generating,
   * parsing, or validating a stream of packets that share one complete Packet
   * Identification value.
   *
   * @par Finalization
   * Setters mark the packet dirty. Getters only inspect the currently stored
   * state and never call update(). update() recalculates dependent fields without
   * producing bytes. serialize() calls update() and returns the finalized wire
   * representation. Packet finalization accepts only encoded Packet Version
   * Number 000, as required by CCSDS 133.0-B-2.
   *
   * @par Idle packets
   * APID 0x7FF is reserved for Idle Packets. A serializable Idle Packet must use
   * Secondary Header Flag zero, contain no secondary-header object, and carry at
   * least one octet of mission-defined idle user data. CCSDSPack does not validate
   * the mission-specific idle fill pattern.
   *
   * @par Parsing boundaries
   * deserializeBounded() reads exactly the length declared by the primary header
   * and returns the number of consumed bytes. Any bytes after that boundary remain
   * unconsumed, allowing callers to iterate over concatenated packets.
   *
   * @code{.cpp}
   * CCSDS::Packet packet;
   * packet.setPrimaryHeader({0, 0, 0, 0x123,
   *                          CCSDS::UNSEGMENTED, 0, 0});
   * packet.setApplicationData({0x10, 0x20});
   * const auto wire = packet.serialize();
   *
   * CCSDS::Packet decoded;
   * const auto consumed = decoded.deserializeBounded(wire);
   * @endcode
   */
  class Packet {
  public:
    /** @brief Constructs an empty packet using the CCSDSPack CRC16 profile. */
    Packet() = default;

    /**
     * @brief Replaces all primary-header fields from a field structure.
     * @param data Field values to validate and assign atomically.
     * @return Success, or INVALID_HEADER_DATA when any field exceeds its bit width.
     * @note The supplied Packet Data Length may later be replaced by update(). A
     * non-zero Packet Version Number remains inspectable but prevents Packet serialization.
     */
    ResultBool setPrimaryHeader(PrimaryHeader data);

    /**
     * @brief Replaces the primary header from the low 48 bits of a packed value.
     * @param data Packed CCSDS primary-header value.
     * @return Success, or an error when the encoded fields are invalid.
     * @note Packet serialization accepts only encoded Packet Version Number zero.
     */
    [[nodiscard]] ResultBool setPrimaryHeader(std::uint64_t data);

    /**
     * @brief Replaces the primary header from exactly six serialized bytes.
     * @param data Big-endian CCSDS primary-header bytes.
     * @return Success, or INVALID_HEADER_DATA for an invalid byte count or fields.
     * @note Packet serialization accepts only encoded Packet Version Number zero.
     */
    [[nodiscard]] ResultBool setPrimaryHeader(const std::vector<std::uint8_t> &data);

    /**
     * @brief Copies an already constructed Header into this packet.
     * @param header Header instance to copy.
     * @note An invalid or non-zero-version copied header prevents Packet serialization.
     */
    void setPrimaryHeader(const Header &header);

    /**
     * @brief Installs a secondary-header object and synchronizes the primary-header flag.
     * @param header Shared secondary-header instance, or nullptr to remove it.
     * @note The Packet shares ownership of the supplied object. Idle Packets may not serialize with one.
     */
    void setDataFieldHeader(const std::shared_ptr<SecondaryHeaderAbstract> &header);

    /**
     * @brief Registers a custom secondary-header type for subsequent typed parsing.
     * @tparam T Default-constructible class derived from SecondaryHeaderAbstract.
     * @return Success, or a registration error.
     *
     * The registered object's getType() value is the lookup key used by typed
     * setDataFieldHeader() and deserializeBounded() overloads.
     */
    template <typename T>
    ResultBool RegisterSecondaryHeader() {
      FORWARD_RESULT(m_dataField.RegisterSecondaryHeader<T>());
      return true;
    }

    /**
     * @brief Creates a registered secondary-header type from serialized bytes.
     * @param data Bytes belonging only to the secondary header.
     * @param headerType Value returned by the registered type's getType().
     * @return Success, or INVALID_SECONDARY_HEADER_DATA.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &data,
                                                const std::string &headerType);

    /**
     * @brief Creates a registered secondary-header type from a byte span.
     * @param pData Pointer to the first secondary-header byte.
     * @param sizeData Number of bytes in the secondary header.
     * @param headerType Registered type name.
     * @return Success, or an error for a null pointer, size mismatch, or unknown type.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                std::size_t sizeData,
                                                const std::string &headerType);

    /**
     * @brief Stores an opaque raw secondary header using BufferHeader.
     * @param data Secondary-header bytes.
     * @return Success, or INVALID_SECONDARY_HEADER_DATA when the data exceeds capacity.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &data);

    /**
     * @brief Stores an opaque raw secondary header from a byte span.
     * @param pData Pointer to the first secondary-header byte.
     * @param sizeData Number of bytes to copy.
     * @return Success, or an error for a null pointer, empty span, or insufficient capacity.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                std::size_t sizeData);

    /**
     * @brief Replaces the packet application data.
     * @param data Application-data bytes; an empty vector clears the field.
     * @return Success, or INVALID_APPLICATION_DATA when the configured capacity is exceeded.
     */
    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &data);

    /**
     * @brief Replaces application data from a byte span.
     * @param pData Pointer to the first application-data byte.
     * @param sizeData Number of bytes to copy; this overload requires at least one byte.
     * @return Success, or an error for a null pointer, empty span, or insufficient capacity.
     */
    [[nodiscard]] ResultBool setApplicationData(const std::uint8_t *pData,
                                                std::size_t sizeData);

    /**
     * @brief Sets the two-bit segmentation flag in the primary header.
     * @param flags CONTINUING_SEGMENT, FIRST_SEGMENT, LAST_SEGMENT, or UNSEGMENTED.
     */
    void setSequenceFlags(ESequenceFlag flags);

    /**
     * @brief Sets the 14-bit Packet Sequence Count used by the CCSDSPack profile.
     * @param count Value in the inclusive range 0..16383.
     * @return Success, or INVALID_HEADER_DATA for an out-of-range count.
     * @note CCSDSPack uses Packet Sequence Count semantics for both telemetry and
     * telecommand packets; the optional telecommand Packet Name interpretation is not implemented.
     */
    [[nodiscard]] ResultBool setSequenceCount(std::uint16_t count);

    /**
     * @brief Sets the maximum combined secondary-header and application-data capacity.
     * @param size Maximum Packet Data Field content bytes before the optional CRC trailer.
     * @note Existing content is not truncated when the capacity is reduced.
     */
    void setDataFieldSize(std::uint16_t size);

    /**
     * @brief Enables or disables automatic dependent-field finalization.
     * @param enable True to let update()/serialize() refresh secondary-header state,
     * Packet Data Length, sequence count, and CRC; false to preserve stored values.
     */
    void setUpdatePacketEnable(bool enable);

    /**
     * @brief Selects the CCSDSPack mission-profile CRC trailer behavior.
     * @param mode None or CRC16.
     * @note Configure this before deserializing a CRC-free packet. CRC trailer
     * octets are part of the CCSDS Packet Data Field and Packet Data Length.
     */
    void setPacketErrorControlMode(PacketErrorControlMode mode);

    /** @brief Returns the configured mission-profile CRC trailer mode. */
    [[nodiscard]] PacketErrorControlMode getPacketErrorControlMode() const {
      return (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK) != 0U
               ? PacketErrorControlMode::None
               : PacketErrorControlMode::CRC16;
    }

    /** @brief Returns the reserved trailer size: two bytes for CRC16, otherwise zero. */
    [[nodiscard]] std::uint16_t getPacketErrorControlSize() const {
      return getPacketErrorControlMode() == PacketErrorControlMode::CRC16 ? 2U : 0U;
    }

    /**
     * @brief Parses one packet using the currently configured CRC profile.
     * @param data Buffer beginning with a CCSDS primary header.
     * @return Success, or a deterministic header, length, data, or checksum error.
     * @note Trailing bytes are ignored. Use deserializeBounded() to learn the boundary.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data);

    /**
     * @brief Parses one packet and decodes a registered secondary-header type.
     * @param data Buffer beginning with a CCSDS packet.
     * @param headerType Registered secondary-header type name.
     * @param headerSize Explicit PusC header size when positive; otherwise the type's getSize().
     * @return Success, or a parsing/secondary-header error.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data,
                                         const std::string &headerType,
                                         std::int32_t headerSize = -1);

    /**
     * @brief Parses one packet using an explicit opaque secondary-header byte count.
     * @param data Buffer beginning with a CCSDS packet.
     * @param headerDataSizeBytes Number of Packet Data Field bytes assigned to BufferHeader.
     * @return Success, or a parsing error when the declared boundary is invalid.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data,
                                         std::uint16_t headerDataSizeBytes);

    /**
     * @brief Parses an already separated primary header and Packet Data Field.
     * @param headerData Exactly six primary-header bytes.
     * @param data Packet Data Field bytes including the configured CRC trailer.
     * @return Success, or a header, length, or checksum error.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &headerData,
                                         const std::vector<std::uint8_t> &data);

    /**
     * @brief Parses exactly one packet and returns its declared serialized size.
     * @param data Buffer beginning with a packet; additional bytes may follow.
     * @return Consumed byte count on success, or a deterministic parsing error.
     */
    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data);

    /**
     * @brief Bounded parse with a registered secondary-header type.
     * @param data Buffer beginning with a packet.
     * @param headerType Registered secondary-header type name.
     * @param headerSize Explicit PusC header size when positive; otherwise the type's getSize().
     * @return Consumed byte count on success, or a parsing/secondary-header error.
     */
    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data,
                                                         const std::string &headerType,
                                                         std::int32_t headerSize = -1);

    /**
     * @brief Bounded parse using an explicit opaque secondary-header byte count.
     * @param data Buffer beginning with a packet.
     * @param headerDataSizeBytes Number of data-field bytes belonging to the secondary header.
     * @return Consumed byte count on success, or a boundary/parsing error.
     */
    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data,
                                                         std::uint16_t headerDataSizeBytes);

    /** @brief Returns the currently stored packed 48-bit primary header without finalizing. */
    std::uint64_t getPrimaryHeader64bit();
    /** @brief Const overload of getPrimaryHeader64bit(). */
    [[nodiscard]] std::uint64_t getPrimaryHeader64bit() const;

    /**
     * @brief Returns the current packet size through the legacy 16-bit API.
     * @return Exact size through 65535 bytes, otherwise UINT16_MAX instead of wrapping.
     * @note Use getSerializedSize() for the complete CCSDS range up to 65542 bytes.
     */
    std::uint16_t getFullPacketLength();
    /** @brief Const overload of getFullPacketLength(). */
    [[nodiscard]] std::uint16_t getFullPacketLength() const;

    /**
     * @brief Returns the exact current serialized size without finalizing the packet.
     * @return Six primary-header bytes plus stored data-field content and configured CRC trailer size.
     * @note The maximum CCSDS Space Packet size is 65542 bytes.
     */
    [[nodiscard]] std::size_t getSerializedSize() const {
      return 6U + static_cast<std::size_t>(m_dataField.getDataFieldUsedBytesSize())
             + static_cast<std::size_t>(getPacketErrorControlSize());
    }

    /**
     * @brief Finalizes and serializes the packet under the v1.2 PDU profile.
     * @return Complete wire bytes, or an empty vector when the header, version,
     * Idle Packet structure, size, or update state is invalid.
     */
    std::vector<std::uint8_t> serialize();

    /** @brief Returns the currently stored six primary-header bytes without finalizing. */
    std::vector<std::uint8_t> getPrimaryHeaderBytes();
    /** @brief Const overload of getPrimaryHeaderBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getPrimaryHeaderBytes() const;

    /** @brief Returns the currently stored secondary-header bytes without finalizing. */
    std::vector<std::uint8_t> getDataFieldHeaderBytes();
    /** @brief Const overload of getDataFieldHeaderBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getDataFieldHeaderBytes() const;

    /** @brief Returns a copy of the current application-data bytes without finalizing. */
    std::vector<std::uint8_t> getApplicationDataBytes();
    /** @brief Const overload of getApplicationDataBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getApplicationDataBytes() const;

    /**
     * @brief Returns the internal secondary-header plus application-data bytes.
     * @note The optional CRC trailer is omitted here even though it occupies the
     * final octets of the wire-level CCSDS Packet Data Field.
     */
    std::vector<std::uint8_t> getFullDataFieldBytes();
    /** @brief Const overload of getFullDataFieldBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getFullDataFieldBytes() const;

    /** @brief Returns the stored CRC trailer as two big-endian bytes, or empty in None mode. */
    std::vector<std::uint8_t> getCRCVectorBytes();
    /** @brief Const overload of getCRCVectorBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getCRCVectorBytes() const;

    /** @brief Returns the stored CRC trailer value, or zero when disabled or the header is invalid. */
    std::uint16_t getCRC();
    /** @brief Const overload of getCRC(). */
    [[nodiscard]] std::uint16_t getCRC() const;

    /** @brief Returns remaining configured internal data-field capacity before the CRC trailer. */
    [[nodiscard]] std::uint16_t getDataFieldMaximumSize() const;

    /** @brief Returns whether the primary header currently asserts a secondary header. */
    bool getDataFieldHeaderFlag();
    /** @brief Const overload of getDataFieldHeaderFlag(). */
    [[nodiscard]] bool getDataFieldHeaderFlag() const;

    /**
     * @brief Returns mutable access to the owned DataField.
     * @warning Direct mutation can bypass Packet setter bookkeeping and profile checks; prefer Packet setters.
     */
    DataField &getDataField();
    /** @brief Returns read-only access to the owned DataField. */
    [[nodiscard]] const DataField &getDataField() const;

    /**
     * @brief Returns mutable access to the owned primary Header.
     * @warning Direct mutation can bypass Packet sequence-cache bookkeeping; prefer Packet setters.
     */
    Header &getPrimaryHeader();
    /** @brief Returns read-only access to the owned primary Header. */
    [[nodiscard]] const Header &getPrimaryHeader() const;

    /**
     * @brief Replaces all CRC16 parameters and marks the packet dirty.
     * @param crcConfig Polynomial, initial value, and final XOR configuration.
     */
    void setCrcConfig(const CRC16Config crcConfig) {
      m_CRC16Config = crcConfig;
      m_updateStatus = false;
    }

    /**
     * @brief Replaces the CRC16 parameters and marks the packet dirty.
     * @param polynomial CRC generator polynomial.
     * @param initialValue Initial CRC register value.
     * @param finalXorValue Value XORed with the final CRC.
     */
    void setCrcConfig(const std::uint16_t polynomial,
                      const std::uint16_t initialValue,
                      const std::uint16_t finalXorValue) {
      m_CRC16Config = {polynomial, initialValue, finalXorValue};
      m_updateStatus = false;
    }

    /**
     * @brief Finalizes dependent packet state without returning serialized bytes.
     *
     * When updates are enabled and the packet satisfies the v1.2 profile, this
     * refreshes the secondary header, encodes Packet Data Length as Packet Data
     * Field octets minus one, writes the cached sequence count, and recalculates
     * the optional CRC16 trailer over the finalized header and preceding data.
     */
    void update();

    /**
     * @brief Loads packet construction settings from a configuration file.
     * @param configPath Path to a CCSDSPack key:type=value configuration file.
     * @return Success, or CONFIG_FILE_ERROR/field-specific validation errors.
     * @note ccsds_version_number must be zero for this Space Packet profile.
     */
    ResultBool loadFromConfigFile(const std::string &configPath);
#ifndef CCSDS_MCU
    /**
     * @brief Loads packet construction settings from an already parsed Config.
     * @param cfg Configuration object containing the required CCSDS header keys.
     * @return Success, or a configuration/field validation error.
     * @note ccsds_version_number must be zero for this Space Packet profile.
     */
    ResultBool loadFromConfig(const Config &cfg);
#endif

  private:
    static constexpr std::uint16_t SEQUENCE_COUNT_MASK = 0x3FFFU;
    static constexpr std::uint16_t PACKET_ERROR_CONTROL_DISABLED_MASK = 0x8000U;

    Header m_primaryHeader{};
    DataField m_dataField{};
    std::uint16_t m_CRC16{};
    CRC16Config m_CRC16Config;
    bool m_updateStatus{false};
    bool m_enableUpdatePacket{true};
    std::uint16_t m_sequenceCounter{0};
  };
}

#endif // CCSDS_PACKET_H
