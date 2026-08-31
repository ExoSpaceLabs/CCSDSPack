// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_VALIDATOR_H
#define CCSDS_VALIDATOR_H

#include <array>
#include <cstddef>
#include <cstdint>
#include "CCSDSPacket.h"

namespace ccsds {

  /** @brief Stable validation checks exposed by ccsds::Validator. */
  enum class ValidationCode : std::uint8_t {
    PrimaryHeader = 0,
    PacketVersion,
    PacketDataLength,
    Crc16,
    SecondaryHeaderPresence,
    SecondaryHeaderDirection,
    SequenceFlags,
    SequenceCount,
    PacketIdentifier,
    SegmentationClass,
    TemplatePacketErrorControl,
    TemplateSecondaryHeader,
    PusHeader,
    PusRevision,
    PusDirection,
    PusPacketType,
    PusTailoring,
    PusSecondaryHeaderSize,
    PusReservedBits,
    PusSpareFields,
    PusAcknowledgement,
    PusSourceId,
    PusDestinationId,
    PusPacketSubcounter,
    PusTimeReferenceStatus,
    PusTimestamp
  };

  [[nodiscard]] const char *validationCodeName(ValidationCode code) noexcept;

  struct ValidationCheck {
    ValidationCode code{ValidationCode::PrimaryHeader};
    bool passed{true};
  };

  /** @brief Fixed-capacity structured validation result with no dynamic allocation. */
  class ValidationReport {
  public:
    static constexpr std::size_t Capacity{32U};

    [[nodiscard]] bool valid() const noexcept {
      for (std::size_t i = 0U; i < m_size; ++i) if (!m_checks[i].passed) return false;
      return true;
    }
    [[nodiscard]] explicit operator bool() const noexcept { return valid(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_size; }
    [[nodiscard]] bool contains(const ValidationCode code) const noexcept {
      for (std::size_t i = 0U; i < m_size; ++i) if (m_checks[i].code == code) return true;
      return false;
    }
    [[nodiscard]] bool passed(const ValidationCode code) const noexcept {
      for (std::size_t i = 0U; i < m_size; ++i)
        if (m_checks[i].code == code) return m_checks[i].passed;
      return false;
    }
    [[nodiscard]] bool failed(const ValidationCode code) const noexcept {
      return contains(code) && !passed(code);
    }
    [[nodiscard]] const ValidationCheck *begin() const noexcept { return m_checks.data(); }
    [[nodiscard]] const ValidationCheck *end() const noexcept { return m_checks.data() + m_size; }

  private:
    void set(const ValidationCode code, const bool passed) noexcept {
      for (std::size_t i = 0U; i < m_size; ++i) {
        if (m_checks[i].code == code) {
          m_checks[i].passed = m_checks[i].passed && passed;
          return;
        }
      }
      if (m_size < Capacity) m_checks[m_size++] = {code, passed};
    }
    std::array<ValidationCheck, Capacity> m_checks{};
    std::size_t m_size{0U};
    friend class Validator;
  };

  /**
   * @brief Validates packet coherence, concrete secondary-header contracts, and one sequence stream.
   *
   * Packet validation is read-only. PUS identity/tailoring is read directly from the
   * installed concrete PUS header; no independent MissionProfile exists.
   */
  class Validator {
  public:
    Validator() = default;
    ~Validator() = default;
    explicit Validator(const Packet &templatePacket) : m_templatePacket(templatePacket) {}

    void setTemplatePacket(const Packet &templatePacket) { m_templatePacket = templatePacket; }
    void configure(bool validatePacketCoherence,
                   bool validateSequenceCount,
                   bool validateAgainstTemplate);
    [[nodiscard]] ValidationReport validate(const Packet &packet);
    [[nodiscard]] const ValidationReport &getReport() const noexcept { return m_report; }
    void clear();

  private:
    static constexpr std::uint16_t SEQUENCE_COUNT_MASK{0x3FFFU};
    static constexpr std::uint16_t SEGMENT_OPEN_MASK{0x4000U};
    static constexpr std::uint16_t SEQUENCE_INITIALIZED_MASK{0x8000U};

    [[nodiscard]] bool sequenceInitialized() const noexcept {
      return (m_sequenceCounter & SEQUENCE_INITIALIZED_MASK) != 0U;
    }
    [[nodiscard]] bool segmentOpen() const noexcept {
      return (m_sequenceCounter & SEGMENT_OPEN_MASK) != 0U;
    }
    [[nodiscard]] std::uint16_t expectedSequenceCount() const noexcept {
      return m_sequenceCounter & SEQUENCE_COUNT_MASK;
    }
    void acceptSequence(const Header &header) noexcept;
    void setCheck(ValidationCode code, bool passed) noexcept { m_report.set(code, passed); }

    Packet m_templatePacket;
    bool m_validatePacketCoherence{true};
    bool m_validateAgainstTemplate{false};
    bool m_validateSequenceCount{true};
    std::uint16_t m_sequenceCounter{0U};
    ValidationReport m_report{};
  };
} // namespace ccsds

#endif // CCSDS_VALIDATOR_H
