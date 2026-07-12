// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_VALIDATOR_H
#define CCSDS_VALIDATOR_H

#include "CCSDSPacket.h"

namespace CCSDS {
  class Validator {
  public:
    Validator() = default;
    ~Validator() = default;

    explicit Validator(const Packet &templatePacket) : m_templatePacket(templatePacket) {}

    void setTemplatePacket(const Packet &templatePacket) {
      m_templatePacket = templatePacket;
    }

    void configure(bool validatePacketCoherence,
                   bool validateSequenceCount,
                   bool validateAgainstTemplate);

    bool validate(const Packet &packet);

    /**
     * Report indices:
     * [0] Packet Data Length coherence
     * [1] CRC coherence
     * [2] sequence-flag coherence
     * [3] sequence-count continuity
     * [4] complete Packet Identification matches template
     * [5] segmented/unsegmented class matches template
     */
    [[nodiscard]] std::vector<bool> getReport() const { return m_report; }

    void clear();

  private:
    static constexpr std::uint16_t SEQUENCE_COUNT_MASK{0x3FFFU};
    static constexpr std::uint16_t SEGMENT_OPEN_MASK{0x4000U};
    static constexpr std::uint16_t SEQUENCE_INITIALIZED_MASK{0x8000U};

    [[nodiscard]] bool sequenceInitialized() const {
      return (m_sequenceCounter & SEQUENCE_INITIALIZED_MASK) != 0U;
    }
    [[nodiscard]] bool segmentOpen() const {
      return (m_sequenceCounter & SEGMENT_OPEN_MASK) != 0U;
    }
    [[nodiscard]] std::uint16_t expectedSequenceCount() const {
      return m_sequenceCounter & SEQUENCE_COUNT_MASK;
    }
    void acceptSequence(const Header &header);

    Packet m_templatePacket;
    bool m_validatePacketCoherence{true};
    bool m_validateAgainstTemplate{false};
    bool m_validateSegmentedCount{true};
    std::uint16_t m_sequenceCounter{0};
    std::vector<bool> m_report{};
    std::size_t m_reportSize{6};
    CRC16Config m_CRCConfig;
  };
}

#endif // CCSDS_VALIDATOR_H
