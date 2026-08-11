// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSValidator.h
 * @brief Defines structured, stateful CCSDS and PUS packet validation.
 */
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
    PacketErrorControlProfile,
    Crc16,
    SecondaryHeaderPresence,
    SequenceFlags,
    SequenceCount,
    PacketIdentifier,
    SegmentationClass,
    MissionProfile,
    TemplateMissionProfile,
    PusHeader,
    PusRevision,
    PusDirection,
    PusPacketType,
    PusProfile,
    PusSecondaryHeaderSize,
    PusReservedBits,
    PusSpareFields,
    PusAcknowledgement,
    PusSourceId,
    PusDestinationId,
    PusTimeReferenceStatus,
    PusTimestamp
  };

  /** @brief Returns a stable printable label for one validation code. */
  [[nodiscard]] const char *validationCodeName(ValidationCode code) noexcept;

  /** @brief One performed validation check. */
  struct ValidationCheck {
    ValidationCode code{ValidationCode::PrimaryHeader};
    bool passed{true};
  };

  /**
   * @brief Fixed-capacity structured validation result.
   *
   * The report intentionally uses std::array rather than a dynamically allocated
   * container so the diagnostic surface remains deterministic in MCU/bare-metal
   * builds. Only checks that were actually performed are present.
   */
  class ValidationReport {
  public:
    static constexpr std::size_t Capacity{32U};

    /** @brief Returns true when every performed check passed. */
    [[nodiscard]] bool valid() const noexcept {
      for (std::size_t i = 0U; i < m_size; ++i) {
        if (!m_checks[i].passed) return false;
      }
      return true;
    }

    /** @brief Contextual success conversion. */
    [[nodiscard]] explicit operator bool() const noexcept { return valid(); }

    /** @brief Number of performed checks stored in the report. */
    [[nodiscard]] std::size_t size() const noexcept { return m_size; }

    /** @brief Returns true when the named check was performed. */
    [[nodiscard]] bool contains(const ValidationCode code) const noexcept {
      for (std::size_t i = 0U; i < m_size; ++i) {
        if (m_checks[i].code == code) return true;
      }
      return false;
    }

    /** @brief Returns true only when the named check exists and passed. */
    [[nodiscard]] bool passed(const ValidationCode code) const noexcept {
      for (std::size_t i = 0U; i < m_size; ++i) {
        if (m_checks[i].code == code) return m_checks[i].passed;
      }
      return false;
    }

    /** @brief Returns true only when the named check exists and failed. */
    [[nodiscard]] bool failed(const ValidationCode code) const noexcept {
      return contains(code) && !passed(code);
    }

    /** @brief Returns the first performed check. */
    [[nodiscard]] const ValidationCheck *begin() const noexcept {
      return m_checks.data();
    }

    /** @brief Returns one-past-the-last performed check. */
    [[nodiscard]] const ValidationCheck *end() const noexcept {
      return m_checks.data() + m_size;
    }

  private:
    void set(const ValidationCode code, const bool passed) noexcept {
      for (std::size_t i = 0U; i < m_size; ++i) {
        if (m_checks[i].code == code) {
          m_checks[i].passed = m_checks[i].passed && passed;
          return;
        }
      }
      if (m_size >= Capacity) return;
      m_checks[m_size++] = {code, passed};
    }

    std::array<ValidationCheck, Capacity> m_checks{};
    std::size_t m_size{0U};

    friend class Validator;
  };

  /**
   * @class Validator
   * @brief Validates packet coherence, profiles, PUS fields, and one sequence stream.
   *
   * Validator is C++17-only, exception-free, and does not require RTTI. The
   * ValidationReport itself performs no dynamic allocation. Packet validation is
   * read-only; only the Validator's sequence-tracking state changes between calls.
   */
  class Validator {
  public:
    /** @brief Constructs a validator with coherence and sequence checks enabled. */
    Validator() = default;

    /** @brief Destroys the validator. */
    ~Validator() = default;

    /**
     * @brief Constructs a validator with a packet template.
     * @param templatePacket Template copied for optional identifier/profile comparison.
     * @note Call configure() to enable template comparison.
     */
    explicit Validator(const Packet &templatePacket) : m_templatePacket(templatePacket) {}

    /**
     * @brief Replaces the comparison template.
     * @param templatePacket Packet whose identifier, profile, and segmentation class form the template.
     */
    void setTemplatePacket(const Packet &templatePacket) { m_templatePacket = templatePacket; }

    /**
     * @brief Configures the validation groups.
     * @param validatePacketCoherence Validate primary header, length, CRC, profile, PUS, and segmentation.
     * @param validateSequenceCount Validate modulo-16384 sequence continuity.
     * @param validateAgainstTemplate Compare packet identification, profile, and segmentation class.
     */
    void configure(bool validatePacketCoherence,
                   bool validateSequenceCount,
                   bool validateAgainstTemplate);

    /**
     * @brief Validates one packet and returns named checks instead of report indices.
     * @param packet Packet to inspect; the packet, profile, and secondary header are not mutated.
     * @return Fixed-capacity structured report.
     *
     * Sequence state advances only after all enabled checks for the packet pass.
     */
    [[nodiscard]] ValidationReport validate(const Packet &packet);

    /** @brief Returns the most recent structured report. */
    [[nodiscard]] const ValidationReport &getReport() const noexcept { return m_report; }

    /**
     * @brief Clears the latest report, sequence state, and stored template.
     * @note Validation enable flags are retained.
     */
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

    Packet m_templatePacket;               ///< Template used for identifier/profile comparison.
    bool m_validatePacketCoherence{true};  ///< Enables structural/profile/PUS checks.
    bool m_validateAgainstTemplate{false}; ///< Enables template checks.
    bool m_validateSequenceCount{true};    ///< Enables count continuity.
    std::uint16_t m_sequenceCounter{0U};   ///< Expected count plus initialized/open-segment flags.
    ValidationReport m_report{};           ///< Most recent structured validation report.
  };
} // namespace ccsds

#endif // CCSDS_VALIDATOR_H
