// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_HEADER_H
#define CCSDS_HEADER_H

#include <CCSDSResult.h>
#include <cstdint>
#include <vector>

namespace CCSDS {
  inline constexpr std::uint16_t IDLE_APID = 0x07FFU;

  enum ESequenceFlag : std::uint8_t {
    CONTINUING_SEGMENT,
    FIRST_SEGMENT,
    LAST_SEGMENT,
    UNSEGMENTED
  };

  enum EHeaderStatus : std::uint8_t {
    NORMAL,
    IDLE,
    INVALID
  };

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

    std::vector<std::uint8_t> serialize();
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    std::uint64_t getFullHeader();
    [[nodiscard]] std::uint64_t getFullHeader() const;

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
