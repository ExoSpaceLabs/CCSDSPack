// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSPacket.h
 * @brief Defines the high-level CCSDS Space Packet container and packet error-control configuration.
 *
 * A Packet owns one primary header, one packet data field, and an optional packet
 * error-control field. Inspection APIs are non-mutating. Call update() or
 * serialize() when dependent fields such as Packet Data Length, sequence count,
 * secondary-header contents, or CRC16 must be finalized.
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
   * @brief Parameters used by the packet CRC-16 calculation and validation paths.
   *
   * The defaults implement CRC-16/CCITT-FALSE: polynomial 0x1021, initial value
   * 0xFFFF, and no final XOR. The CRC covers the serialized six-byte primary
   * header followed by the complete packet data field, excluding the CRC bytes.
   */
  struct CRC16Config {
    std::uint16_t polynomial = 0x1021;   ///< CRC generator polynomial.
    std::uint16_t initialValue = 0xFFFF; ///< Initial CRC register value.
    std::uint16_t finalXorValue = 0x0000;///< Value XORed with the final CRC.
  };

  /**
   * @enum PacketErrorControlMode
   * @brief Selects whether a packet carries a two-byte CRC16 error-control field.
   *
   * Parsing never infers the mode from trailing bytes. Configure the receiving
   * Packet before deserialization when the stream does not use the default CRC16
   * mode.
   */
  enum class PacketErrorControlMode : std::uint8_t {
    None = 0, ///< No packet error-control bytes are serialized or validated.
    CRC16 = 1 ///< Append and validate a two-byte CRC16 field. This is the default.
  };

  /**
   * @class Packet
   * @brief Owns, serializes, and parses one CCSDS Space Packet.
   *
   * Packet provides checked primary-header assignment, optional typed or raw
   * secondary headers, application data, bounded parsing, CRC16 handling, and
   * explicit finalization. The object represents exactly one packet. Use Manager
   * when generating, parsing, or validating a stream of packets that share one
   * complete Packet Identification value.
   *
   * @par Finalization
   * Setters mark the packet dirty. Getters only inspect the currently stored
   * state and never call update(). update() recalculates dependent fields without
   * producing bytes. serialize() calls update() and returns the finalized wire
   * representation.
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
    /** @brief Constructs an empty packet using CRC16 packet error control. */
    Packet() = default;

    /**
     * @brief Replaces all primary-header fields from a field structure.
     * @param data Field values to validate and assign atomically.
     * @return Success, or INVALID_HEADER_DATA when any field exceeds its CCSDS width.
     * @note The supplied Packet Data Length may later be replaced by update().
     */
    ResultBool setPrimaryHeader(PrimaryHeader data);

    /**
     * @brief Replaces the primary header from the low 48 bits of a packed value.
     * @param data Packed CCSDS primary-header value.
     * @return Success, or an error when the encoded fields are invalid.
     */
    [[nodiscard]] ResultBool setPrimaryHeader(std::uint64_t data);

    /**
     * @brief Replaces the primary header from exactly six serialized bytes.
     * @param data Big-endian CCSDS primary-header bytes.
     * @return Success, or INVALID_HEADER_DATA for an invalid byte count or fields.
     */
    [[nodiscard]] ResultBool setPrimaryHeader(const std::vector<std::uint8_t> &data);

    /**
     * @brief Copies an already constructed Header into this packet.
     * @param header Header instance to copy.
     * @note An invalid copied header prevents serialization until corrected.
     */
    void setPrimaryHeader(const Header &header);

    /**
     * @brief Installs a secondary-header object and synchronizes the primary-header flag.
     * @param header Shared secondary-header instance, or nullptr to remove it.
     * @note The Packet shares ownership of the supplied object.
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
     * @brief Sets the 14-bit packet sequence count.
     * @param count Value in the inclusive range 0..16383.
     * @return Success, or INVALID_HEADER_DATA for an out-of-range count.
     */
    [[nodiscard]] ResultBool setSequenceCount(std::uint16_t count);

    /**
     * @brief Sets the maximum combined secondary-header and application-data capacity.
     * @param size Maximum packet data-field content bytes before optional PEC bytes.
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
     * @brief Selects packet error-control behavior for serialization and parsing.
     * @param mode None or CRC16.
     * @note Configure this before deserializing a CRC-free packet.
     */
    void setPacketErrorControlMode(PacketErrorControlMode mode);

    /** @brief Returns the configured packet error-control mode. */
    [[nodiscard]] PacketErrorControlMode getPacketErrorControlMode() const {
      return (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK) != 0U
               ? PacketErrorControlMode::None
               : PacketErrorControlMode::CRC16;
    }

    /** @brief Returns the serialized PEC size: two bytes for CRC16, otherwise zero. */
    [[nodiscard]] std::uint16_t getPacketErrorControlSize() const {
      return getPacketErrorControlMode() == PacketErrorControlMode::CRC16 ? 2U : 0U;
    }

    /**
     * @brief Parses one packet using the currently configured PEC mode.
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
     * @param headerDataSizeBytes Number of packet-data-field bytes assigned to BufferHeader.
     * @return Success, or a parsing error when the declared boundary is invalid.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data,
                                         std::uint16_t headerDataSizeBytes);

    /**
     * @brief Parses an already separated primary header and packet data field.
     * @param headerData Exactly six primary-header bytes.
     * @param data Packet data-field bytes including the configured PEC bytes.
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
     * @brief Returns the current logical packet size without finalizing the packet.
     * @return Six header bytes plus stored data-field bytes and configured PEC size.
     */
    std::uint16_t getFullPacketLength();
    /** @brief Const overload of getFullPacketLength(). */
    [[nodiscard]] std::uint16_t getFullPacketLength() const;

    /**
     * @brief Finalizes and serializes the packet.
     * @return Complete wire bytes, or an empty vector when the header, size, or update state is invalid.
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

    /** @brief Returns secondary-header bytes followed by application data, excluding PEC bytes. */
    std::vector<std::uint8_t> getFullDataFieldBytes();
    /** @brief Const overload of getFullDataFieldBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getFullDataFieldBytes() const;

    /** @brief Returns the currently stored CRC as two big-endian bytes, or an empty vector in None mode. */
    std::vector<std::uint8_t> getCRCVectorBytes();
    /** @brief Const overload of getCRCVectorBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getCRCVectorBytes() const;

    /** @brief Returns the currently stored CRC value, or zero when PEC is disabled or the header is invalid. */
    std::uint16_t getCRC();
    /** @brief Const overload of getCRC(). */
    [[nodiscard]] std::uint16_t getCRC() const;

    /** @brief Returns remaining configured packet data-field capacity. */
    [[nodiscard]] std::uint16_t getDataFieldMaximumSize() const;

    /** @brief Returns whether a secondary header is currently installed. */
    bool getDataFieldHeaderFlag();
    /** @brief Const overload of getDataFieldHeaderFlag(). */
    [[nodiscard]] bool getDataFieldHeaderFlag() const;

    /**
     * @brief Returns mutable access to the owned DataField.
     * @warning Direct mutation can bypass Packet setter bookkeeping; prefer Packet setters.
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
     * When updates are enabled, this refreshes the secondary header, encodes
     * Packet Data Length as packet-data-field octets minus one, writes the cached
     * sequence count, and recalculates CRC16 over the finalized header and data.
     */
    void update();

    /**
     * @brief Loads packet construction settings from a configuration file.
     * @param configPath Path to a CCSDSPack key:type=value configuration file.
     * @return Success, or CONFIG_FILE_ERROR/field-specific validation errors.
     */
    ResultBool loadFromConfigFile(const std::string &configPath);
#ifndef CCSDS_MCU
    /**
     * @brief Loads packet construction settings from an already parsed Config.
     * @param cfg Configuration object containing the required CCSDS header keys.
     * @return Success, or a configuration/field validation error.
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
