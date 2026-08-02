// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSHeader.h
 * @brief Defines CCSDS Space Packet primary-header fields, validation, and serialization.
 */
#ifndef CCSDS_HEADER_H
#define CCSDS_HEADER_H

#include <CCSDSResult.h>
#include <cstdint>
#include <vector>

namespace CCSDS {
  /** @brief Reserved 11-bit APID value identifying an idle Space Packet. */
  inline constexpr std::uint16_t IDLE_APID = 0x07FFU;

  /**
   * @enum ESequenceFlag
   * @brief Values of the two-bit CCSDS Space Packet sequence-flags field.
   *
   * The flags describe whether a packet is complete or one segment of a larger
   * application-data sequence. They belong to the Space Packet primary header,
   * not to a transfer-frame header.
   */
  enum ESequenceFlag : std::uint8_t {
    CONTINUING_SEGMENT, ///< Binary 00: an intermediate segment.
    FIRST_SEGMENT,      ///< Binary 01: first segment of a segmented sequence.
    LAST_SEGMENT,       ///< Binary 10: final segment of a segmented sequence.
    UNSEGMENTED         ///< Binary 11: a complete, unsegmented packet.
  };

  /**
   * @enum EHeaderStatus
   * @brief Indicates whether a Header contains a normal, idle, or invalid APID state.
   *
   * INVALID is a fail-safe state entered when checked field assignment fails. An
   * invalid header serializes to an empty byte vector and reports zero as its packed
   * value. A later successful APID/header assignment restores NORMAL or IDLE.
   */
  enum EHeaderStatus : std::uint8_t {
    NORMAL, ///< APID is in the normal range 0..2046 and all fields are valid.
    IDLE,   ///< APID equals the reserved idle value 2047.
    INVALID ///< At least one checked assignment failed; serialization is suppressed.
  };

  /**
   * @struct PrimaryHeader
   * @brief Plain field representation of the six-byte CCSDS Space Packet primary header.
   *
   * Field widths are validated only when the structure is assigned to Header or Packet.
   * Packet Data Length stores the CCSDS encoded value N-1, where N is the total
   * packet-data-field size including optional packet error-control bytes.
   */
  struct PrimaryHeader {
    std::uint8_t versionNumber{};       ///< 3-bit protocol version; v1.2 parsing supports value 0.
    std::uint8_t type{};                ///< 1-bit packet type.
    std::uint8_t dataFieldHeaderFlag{}; ///< 1-bit secondary-header presence flag.
    std::uint16_t APID{};               ///< 11-bit Application Process Identifier.
    std::uint8_t sequenceFlags{};       ///< 2-bit ESequenceFlag value.
    std::uint16_t sequenceCount{};      ///< 14-bit stream sequence count.
    std::uint16_t dataLength{};         ///< Encoded Packet Data Length field, N-1.

    /**
     * @brief Constructs a field structure from explicit values.
     * @param versionNumber_value Protocol version.
     * @param type_value Packet type.
     * @param dataFieldHeaderFlag_value Secondary-header flag.
     * @param APID_value Application Process Identifier.
     * @param sequenceFlag_value Sequence flags.
     * @param sequenceCount_value Sequence count.
     * @param dataLength_value Encoded Packet Data Length.
     */
    PrimaryHeader(const std::uint8_t versionNumber_value,
                  const std::uint8_t type_value,
                  const std::uint8_t dataFieldHeaderFlag_value,
                  const std::uint16_t APID_value,
                  const std::uint8_t sequenceFlag_value,
                  const std::uint16_t sequenceCount_value,
                  const std::uint16_t dataLength_value)
      : versionNumber(versionNumber_value),
        type(type_value),
        dataFieldHeaderFlag(dataFieldHeaderFlag_value),
        APID(APID_value),
        sequenceFlags(sequenceFlag_value),
        sequenceCount(sequenceCount_value),
        dataLength(dataLength_value) {}
  };

  /**
   * @class Header
   * @brief Validated in-memory representation of a CCSDS Space Packet primary header.
   *
   * Header stores the seven logical fields of the fixed six-byte primary header and
   * provides checked field setters, packed assignment, byte deserialization, and
   * big-endian serialization. Checked updates are atomic: failed assignments do not
   * partially replace the previous logical field set, but they mark the object INVALID
   * so ignored errors cannot silently produce truncated wire values.
   *
   * @code{.cpp}
   * CCSDS::Header header;
   * header.setAPID(0x123);
   * header.setSequenceFlags(CCSDS::UNSEGMENTED);
   * header.setSequenceCount(42);
   * const auto bytes = header.serialize();
   * @endcode
   */
  class Header {
  public:
    /** @brief Constructs a normal version-0 header with APID 0 and UNSEGMENTED flags. */
    Header() = default;

    /** @brief Returns the stored 3-bit protocol version. */
    [[nodiscard]] std::uint8_t getVersionNumber() const { return m_versionNumber; }
    /** @brief Returns the stored 1-bit packet type. */
    [[nodiscard]] std::uint8_t getType() const { return m_type; }
    /** @brief Returns the stored 1-bit secondary-header presence flag. */
    [[nodiscard]] std::uint8_t getDataFieldHeaderFlag() const { return m_dataFieldHeaderFlag; }
    /** @brief Returns the stored 11-bit Application Process Identifier. */
    [[nodiscard]] std::uint16_t getAPID() const { return m_APID; }
    /** @brief Returns the stored 2-bit sequence-flags value. */
    [[nodiscard]] std::uint8_t getSequenceFlags() const { return m_sequenceFlags; }
    /** @brief Returns the stored 14-bit sequence count. */
    [[nodiscard]] std::uint16_t getSequenceCount() const { return m_sequenceCount; }
    /** @brief Returns the encoded Packet Data Length value N-1. */
    [[nodiscard]] std::uint16_t getDataLength() const { return m_dataLength; }
    /** @brief Returns NORMAL, IDLE, or INVALID for the current header state. */
    [[nodiscard]] EHeaderStatus getHeaderStatus() const { return m_status; }

    /**
     * @brief Serializes the current primary header in CCSDS network-byte order.
     * @return Exactly six bytes, or an empty vector while the header is INVALID.
     * @note This operation does not mutate the logical fields.
     */
    std::vector<std::uint8_t> serialize();
    /** @brief Const overload of serialize(). */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    /**
     * @brief Packs the six-byte header into the low 48 bits of a 64-bit integer.
     * @return Packed header value, or zero while the header is INVALID.
     */
    std::uint64_t getFullHeader();
    /** @brief Const overload of getFullHeader(). */
    [[nodiscard]] std::uint64_t getFullHeader() const;

    /**
     * @brief Sets the 3-bit protocol version.
     * @param value Value in the inclusive range 0..7.
     * @return Success, or INVALID_HEADER_DATA for a wider value.
     */
    [[nodiscard]] ResultBool setVersionNumber(const std::uint8_t &value);

    /**
     * @brief Sets the 1-bit packet type.
     * @param value Value 0 or 1.
     * @return Success, or INVALID_HEADER_DATA for any other value.
     */
    [[nodiscard]] ResultBool setType(const std::uint8_t &value);

    /**
     * @brief Sets the 1-bit secondary-header presence flag.
     * @param value Value 0 or 1.
     * @return Success, or INVALID_HEADER_DATA for any other value.
     */
    [[nodiscard]] ResultBool setDataFieldHeaderFlag(const std::uint8_t &value);

    /**
     * @brief Sets the 11-bit Application Process Identifier.
     * @param value Value in the inclusive range 0..2047.
     * @return Success, or INVALID_HEADER_DATA for a wider value.
     * @note Value 2047 changes getHeaderStatus() to IDLE.
     */
    [[nodiscard]] ResultBool setAPID(const std::uint16_t &value);

    /**
     * @brief Sets the 2-bit sequence-flags field.
     * @param value ESequenceFlag-compatible value in the range 0..3.
     * @return Success, or INVALID_HEADER_DATA for a wider value.
     */
    [[nodiscard]] ResultBool setSequenceFlags(const std::uint8_t &value);

    /**
     * @brief Sets the 14-bit packet sequence count.
     * @param value Value in the inclusive range 0..16383.
     * @return Success, or INVALID_HEADER_DATA for a wider value.
     */
    [[nodiscard]] ResultBool setSequenceCount(const std::uint16_t &value);

    /**
     * @brief Sets the encoded 16-bit Packet Data Length field.
     * @param value Encoded N-1 value.
     * @note Packet normally calculates this field during update()/serialize().
     */
    void setDataLength(const std::uint16_t &value);

    /**
     * @brief Atomically assigns all fields from a packed 48-bit value.
     * @param data Packed header in the low 48 bits.
     * @return Success, or INVALID_HEADER_DATA when decoded fields are rejected.
     */
    [[nodiscard]] ResultBool setData(const std::uint64_t &data);

    /**
     * @brief Atomically parses exactly six serialized primary-header bytes.
     * @param data Big-endian header bytes.
     * @return Success, or INVALID_HEADER_DATA for a wrong size or invalid field.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data);

    /**
     * @brief Atomically assigns all logical fields from PrimaryHeader.
     * @param data Field structure to validate.
     * @return Success, or INVALID_HEADER_DATA for the first invalid-width field.
     */
    [[nodiscard]] ResultBool setData(const PrimaryHeader &data);

  private:
    void refreshStatus();

    std::uint8_t m_versionNumber{};
    std::uint8_t m_type{};
    std::uint8_t m_dataFieldHeaderFlag{};
    std::uint16_t m_APID{};
    std::uint8_t m_sequenceFlags{UNSEGMENTED};
    std::uint16_t m_sequenceCount{};
    std::uint16_t m_packetIdentificationAndVersion{};
    std::uint16_t m_packetSequenceControl{};
    std::uint16_t m_dataLength{};
    EHeaderStatus m_status{NORMAL};
  };
}
#endif // CCSDS_HEADER_H
