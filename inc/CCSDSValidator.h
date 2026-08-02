// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSValidator.h
 * @brief Defines stateful CCSDS packet coherence, sequence, and template validation.
 */
#ifndef CCSDS_VALIDATOR_H
#define CCSDS_VALIDATOR_H

#include "CCSDSPacket.h"

namespace CCSDS {
  /**
   * @class Validator
   * @brief Validates packet structure and one sequence-count stream.
   *
   * Validator can check packet coherence, modulo-16384 sequence continuity, and
   * comparison against a template. Sequence validation is stateful and is evaluated
   * as part of the packet-coherence path: each accepted coherent packet advances the
   * expected count and updates segmented-sequence state. Call clear() before starting
   * an unrelated stream, then restore a template if template comparison is required.
   *
   * Template comparison covers the complete Packet Identification value: version,
   * packet type, data-field-header flag, and APID. It does not require equal sequence
   * flags, sequence count, or Packet Data Length.
   */
  class Validator {
  public:
    /** @brief Constructs a validator with packet-coherence and sequence checks enabled. */
    Validator() = default;

    /** @brief Destroys the validator. */
    ~Validator() = default;

    /**
     * @brief Constructs a validator with a packet template.
     * @param templatePacket Template copied for optional identifier comparison.
     * @note Call configure() to enable template comparison.
     */
    explicit Validator(const Packet &templatePacket) : m_templatePacket(templatePacket) {}

    /**
     * @brief Replaces the comparison template.
     * @param templatePacket Packet whose identifier and segmentation class form the template.
     */
    void setTemplatePacket(const Packet &templatePacket) { m_templatePacket = templatePacket; }

    /**
     * @brief Configures which validation groups are performed.
     * @param validatePacketCoherence Check Packet Data Length, CRC16, and segmentation state.
     * @param validateSequenceCount Check modulo-16384 count continuity inside the coherence group.
     * @param validateAgainstTemplate Compare complete Packet Identification and segmentation class.
     *
     * Disabled checks retain their documented report positions and are initialized as
     * passing so getReport() remains index-stable.
     */
    void configure(bool validatePacketCoherence,
                   bool validateSequenceCount,
                   bool validateAgainstTemplate);

    /**
     * @brief Validates one packet and advances sequence state after coherent input.
     * @param packet Packet to inspect; the packet is not mutated.
     * @return True when every enabled check passes.
     * @note Call getReport() immediately afterward for individual check results.
     */
    bool validate(const Packet &packet);

    /**
     * @brief Returns the latest validation report in a stable six-element layout.
     * @return Boolean results with the following indices:
     *
     * - 0: encoded Packet Data Length matches stored packet-data-field size;
     * - 1: stored/received CRC16 matches recalculation, or PEC is disabled;
     * - 2: sequence flags form a coherent segmentation-state transition;
     * - 3: sequence count equals the expected modulo-16384 value;
     * - 4: complete Packet Identification matches the template;
     * - 5: segmented/unsegmented class matches the template.
     */
    [[nodiscard]] std::vector<bool> getReport() const { return m_report; }

    /**
     * @brief Clears the latest report, sequence state, and stored template.
     * @note Validation enable flags are retained. Call setTemplatePacket() again before
     * using template comparison after clear().
     */
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

    Packet m_templatePacket;               ///< Template used for identifier comparison.
    bool m_validatePacketCoherence{true};  ///< Enables length, CRC, flag, and sequence checks.
    bool m_validateAgainstTemplate{false}; ///< Enables template identifier/class checks.
    bool m_validateSegmentedCount{true};   ///< Enables count continuity within coherence checks.
    std::uint16_t m_sequenceCounter{0};    ///< Expected count plus initialized/open-segment flags.
    std::vector<bool> m_report{};          ///< Most recent six-element validation report.
    std::size_t m_reportSize{6};           ///< Stable number of report entries.
    CRC16Config m_CRCConfig;               ///< CRC parameters used for coherence checks.
  };
} // namespace CCSDS

#endif // CCSDS_VALIDATOR_H
