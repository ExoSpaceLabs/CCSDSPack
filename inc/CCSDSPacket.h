// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSPacket.h
 * @brief Defines the high-level CCSDS Space Packet container and mission-profile trailer configuration.
 *
 * A Packet owns one primary header and one packet data field. CCSDSPack may reserve
 * the final two packet-data-field octets for a CRC16 mission-profile trailer.
 * Inspection APIs are non-mutating. Call update() or serialize() when dependent
 * fields such as Packet Data Length, sequence count, secondary-header contents,
 * or CRC16 must be finalized.
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
   * @brief Parameters used by the optional CCSDSPack CRC16 mission-profile trailer.
   *
   * CCSDS 133.0-B-2 defines the Space Packet as a primary header followed by a
   * packet data field; it does not standardize a separate packet-error-control
   * field. In CRC16 mode CCSDSPack reserves the final two packet-data-field octets
   * for CRC-16/CCITT-FALSE. The defaults use polynomial 0x1021, initial value
   * 0xFFFF, and no final XOR. Coverage begins at the first primary-header octet and
   * ends before the two trailer octets.
   */
  struct CRC16Config {
    std::uint16_t polynomial = 0x1021;   ///< CRC generator polynomial.
    std::uint16_t initialValue = 0xFFFF; ///< Initial CRC register value.
    std::uint16_t finalXorValue = 0x0000;///< Value XORed with the final CRC.
  };

  /**
   * @enum PacketErrorControlMode
   * @brief Selects the optional CCSDSPack CRC16 trailer within the Packet Data Field.
   *
   * Parsing never infers the mode from trailing bytes. Configure the receiving
   * Packet before deserialization when the stream does not use the default CRC16
   * mission profile.
   */
  enum class PacketErrorControlMode : std::uint8_t {
    None = 0, ///< No CCSDSPack CRC16 trailer is serialized or validated.
    CRC16 = 1 ///< Reserve and validate two final Packet Data Field octets. Default.
  };

  /**
   * @class Packet
   * @brief Owns, serializes, and parses one CCSDS 133.0-B-2 Space Packet PDU.
   *
   * Packet provides checked primary-header assignment, optional typed or raw
   * secondary headers, application data, bounded parsing, an optional CRC16
   * mission-profile trailer, and explicit finalization. It implements the Space
   * Packet PDU profile rather than the complete CCSDS Packet Service, Octet String
   * Service, protocol procedures, or PICS.
   *
   * Packet Version Number is fixed to encoded value zero. APID 2047 identifies an
   * Idle Packet and cannot be combined with a secondary header. CCSDSPack uses
   * Packet Sequence Count semantics for both telemetry and telecommand packets;
   * telecommand Packet Name semantics are not implemented.
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
    /** @brief Constructs an empty packet using the CCSDSPack CRC16 mission profile. */
    Packet() = default;

    /**
     * @brief Replaces all primary-header fields from a field structure.
     * @param data Field values to validate and assign atomically.
     * @return Success, or INVALID_HEADER_DATA when the Space Packet profile is violated.
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
     * @return Success, or INVALID_HEADER_DATA for an invalid byte count or profile field.
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
     * @note The Packet shares ownership of the supplied object. Idle Packets reject non-null headers.
     */
    void setDataFieldHeader(const std::shared_ptr<SecondaryHeaderAbstract> &header);

    /**
     * @brief Registers a custom secondary-header type for subsequent typed parsing.
     * @tparam T Default-constructible class derived from SecondaryHeaderAbstract.
     * @return Success, or a registration error.
     */
    template <typename T>
    ResultBool RegisterSecondaryHeader() {
      FORWARD_RESULT(m_dataField.RegisterSecondaryHeader<T>());
      return true;
    }

    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &data,
                                                const std::string &headerType);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                std::size_t sizeData,
                                                const std::string &headerType);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &data);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                std::size_t sizeData);

    /** @brief Replaces the packet application data. */
    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &data);
    /** @brief Replaces application data from a byte span. */
    [[nodiscard]] ResultBool setApplicationData(const std::uint8_t *pData,
                                                std::size_t sizeData);

    /** @brief Sets the two-bit segmentation flag in the primary header. */
    void setSequenceFlags(ESequenceFlag flags);
    /** @brief Sets the 14-bit Packet Sequence Count used by the CCSDSPack profile. */
    [[nodiscard]] ResultBool setSequenceCount(std::uint16_t count);
    /** @brief Sets maximum secondary-header plus application-data capacity. */
    void setDataFieldSize(std::uint16_t size);
    /** @brief Enables or disables automatic dependent-field finalization. */
    void setUpdatePacketEnable(bool enable);

    /**
     * @brief Selects the optional CCSDSPack CRC16 trailer behavior.
     * @param mode None or CRC16.
     * @note Configure this before deserializing a packet produced without the trailer.
     */
    void setPacketErrorControlMode(PacketErrorControlMode mode);

    [[nodiscard]] PacketErrorControlMode getPacketErrorControlMode() const {
      return (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK) != 0U
               ? PacketErrorControlMode::None
               : PacketErrorControlMode::CRC16;
    }

    /** @brief Returns the mission-profile trailer size: two bytes for CRC16, otherwise zero. */
    [[nodiscard]] std::uint16_t getPacketErrorControlSize() const {
      return getPacketErrorControlMode() == PacketErrorControlMode::CRC16 ? 2U : 0U;
    }

    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data,
                                         const std::string &headerType,
                                         std::int32_t headerSize = -1);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data,
                                         std::uint16_t headerDataSizeBytes);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &headerData,
                                         const std::vector<std::uint8_t> &data);

    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data);
    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data,
                                                         const std::string &headerType,
                                                         std::int32_t headerSize = -1);
    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data,
                                                         std::uint16_t headerDataSizeBytes);

    /** @brief Returns the currently stored packed 48-bit primary header without finalizing. */
    std::uint64_t getPrimaryHeader64bit();
    /** @brief Const overload of getPrimaryHeader64bit(). */
    [[nodiscard]] std::uint64_t getPrimaryHeader64bit() const;

    /**
     * @brief Returns the legacy 16-bit logical packet size without finalizing.
     * @return Six header bytes plus stored data-field bytes and configured trailer size.
     * @warning Values above 65535 cannot be represented. Use getSerializedSize().
     */
    std::uint16_t getFullPacketLength();
    /** @brief Const overload of the legacy 16-bit getFullPacketLength(). */
    [[nodiscard]] std::uint16_t getFullPacketLength() const;

    /**
     * @brief Returns the complete serialized packet size without 16-bit overflow.
     * @return Six primary-header octets plus stored data-field content and configured trailer size.
     * @note This inspection API does not finalize or mutate the packet.
     */
    [[nodiscard]] std::size_t getSerializedSize() const {
      return 6U + static_cast<std::size_t>(m_dataField.getDataFieldUsedBytesSize())
             + static_cast<std::size_t>(getPacketErrorControlSize());
    }

    /**
     * @brief Finalizes and serializes the packet.
     * @return Complete wire bytes, or an empty vector when the header, profile, size, or update state is invalid.
     */
    std::vector<std::uint8_t> serialize();

    std::vector<std::uint8_t> getPrimaryHeaderBytes();
    [[nodiscard]] std::vector<std::uint8_t> getPrimaryHeaderBytes() const;
    std::vector<std::uint8_t> getDataFieldHeaderBytes();
    [[nodiscard]] std::vector<std::uint8_t> getDataFieldHeaderBytes() const;
    std::vector<std::uint8_t> getApplicationDataBytes();
    [[nodiscard]] std::vector<std::uint8_t> getApplicationDataBytes() const;
    std::vector<std::uint8_t> getFullDataFieldBytes();
    [[nodiscard]] std::vector<std::uint8_t> getFullDataFieldBytes() const;
    std::vector<std::uint8_t> getCRCVectorBytes();
    [[nodiscard]] std::vector<std::uint8_t> getCRCVectorBytes() const;
    std::uint16_t getCRC();
    [[nodiscard]] std::uint16_t getCRC() const;
    [[nodiscard]] std::uint16_t getDataFieldMaximumSize() const;
    bool getDataFieldHeaderFlag();
    [[nodiscard]] bool getDataFieldHeaderFlag() const;

    DataField &getDataField();
    [[nodiscard]] const DataField &getDataField() const;
    Header &getPrimaryHeader();
    [[nodiscard]] const Header &getPrimaryHeader() const;

    void setCrcConfig(const CRC16Config crcConfig) {
      m_CRC16Config = crcConfig;
      m_updateStatus = false;
    }

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
     * sequence count, and recalculates the optional CRC16 mission-profile trailer.
     */
    void update();

    ResultBool loadFromConfigFile(const std::string &configPath);
#ifndef CCSDS_MCU
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
