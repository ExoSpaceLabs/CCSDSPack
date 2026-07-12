// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_HEADER_H
#define CCSDS_HEADER_H

#include <CCSDSResult.h>
#include <cstdint>
#include <vector>

namespace CCSDS {
  inline constexpr std::uint16_t IDLE_APID = 0x07FFU;

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
    LAST_SEGMENT,       ///< 10 Last segment of a packet.
    UNSEGMENTED         ///< 11 Complete unsegmented packet.
  };

  /**
   * @brief Describes whether the current primary header is normal, idle, or invalid.
   *
   * `INVALID` is used as a fail-safe when a checked mutation fails and its
   * returned error is ignored. An invalid header cannot be serialized.
   */
  enum EHeaderStatus : std::uint8_t {
    NORMAL,
    IDLE,
    INVALID
  };

  /**
   * @struct PrimaryHeader
   * @brief Represents the fields of a CCSDS primary header.
   */
  struct PrimaryHeader {
    std::uint8_t versionNumber{};
    std::uint8_t type{};
    std::uint8_t dataFieldHeaderFlag{};
    std::uint16_t APID{};
    std::uint8_t sequenceFlags{};
    std::uint16_t sequenceCount{};
    std::uint16_t dataLength{};

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
   * @brief Manages the decomposition and manipulation of CCSDS primary headers.
   */
  class Header {
  public:
    Header() = default;

    [[nodiscard]] std::uint8_t getVersionNumber()       const { return m_versionNumber;       }
    [[nodiscard]] std::uint8_t getType()                const { return m_type;                }
    [[nodiscard]] std::uint8_t getDataFieldHeaderFlag() const { return m_dataFieldHeaderFlag; }
    [[nodiscard]] std::uint16_t getAPID()               const { return m_APID;                }
    [[nodiscard]] std::uint8_t getSequenceFlags()       const { return m_sequenceFlags;       }
    [[nodiscard]] std::uint16_t getSequenceCount()      const { return m_sequenceCount;       }
    [[nodiscard]] std::uint16_t getDataLength()         const { return m_dataLength;          }
    [[nodiscard]] EHeaderStatus getHeaderStatus()       const { return m_status;              }

    /**
     * @brief Serializes the current primary header.
     * @return Six header bytes, or an empty vector when the header is invalid.
     */
    std::vector<std::uint8_t> serialize();

    /**
     * @brief Returns the packed 48-bit primary header in a 64-bit value.
     * @return The packed header, or zero when the header is invalid.
     */
    std::uint64_t getFullHeader() {
      if (m_status == INVALID) {
        return 0;
      }
      m_packetSequenceControl = (static_cast<std::uint16_t>(m_sequenceFlags) << 14) | m_sequenceCount;
      m_packetIdentificationAndVersion = (static_cast<std::uint16_t>(m_versionNumber) << 13)
                                         | (static_cast<std::uint16_t>(m_type) << 12)
                                         | (static_cast<std::uint16_t>(m_dataFieldHeaderFlag) << 11)
                                         | m_APID;
      return (static_cast<std::uint64_t>(m_packetIdentificationAndVersion) << 32)
             | (static_cast<std::uint32_t>(m_packetSequenceControl) << 16)
             | m_dataLength;
    }

    [[nodiscard]] ResultBool setVersionNumber(const std::uint8_t &value);
    [[nodiscard]] ResultBool setType(const std::uint8_t &value);
    [[nodiscard]] ResultBool setDataFieldHeaderFlag(const std::uint8_t &value);
    [[nodiscard]] ResultBool setAPID(const std::uint16_t &value);
    [[nodiscard]] ResultBool setSequenceFlags(const std::uint8_t &value);
    [[nodiscard]] ResultBool setSequenceCount(const std::uint16_t &value);
    void setDataLength(const std::uint16_t &value);

    [[nodiscard]] ResultBool setData(const std::uint64_t &data);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data);
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
