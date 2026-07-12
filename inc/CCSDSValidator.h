// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/// @file CCSDSValidator.h
/// @brief Defines the Validator class for CCSDS packet validation.
#ifndef CCSDS_VALIDATOR_H
#define CCSDS_VALIDATOR_H

#include "CCSDSPacket.h"

namespace CCSDS {
  /**
   * @brief Handles validation of CCSDS packets.
   *
   * The Validator class checks packet coherence, sequence-stream coherence,
   * and comparison against a packet template.
   */
  class Validator {
  public:
    /// @brief Default constructor.
    Validator() = default;

    /// @brief Default destructor.
    ~Validator() = default;

    /**
     * @brief Constructs a Validator with a template packet.
     * @param templatePacket The packet template to use for validation.
     */
    explicit Validator(const Packet &templatePacket) : m_templatePacket(templatePacket) {}

    /**
     * @brief Sets the template packet for validation.
     * @param templatePacket The new template packet.
     */
    void setTemplatePacket(const Packet &templatePacket) { m_templatePacket = templatePacket; }

    /**
     * @brief Configures validation options.
     * @param validatePacketCoherence Enables packet length, CRC, and flag-coherence validation.
     * @param validateSequenceCount Enables sequence-count continuity validation.
     * @param validateAgainstTemplate Enables comparison against the template.
     */
    void configure(bool validatePacketCoherence,
                   bool validateSequenceCount,
                   bool validateAgainstTemplate);

    /**
     * @brief Validates a given packet.
     * @param packet The packet to validate.
     * @return True if the packet passes validation, otherwise false.
     */
    bool validate(const Packet &packet);

    /**
     * @brief Returns a report of performed validation checks.
     *
     * - Packet coherence:
     *     - index [0]: Packet Data Length matches the packet data field.
     *     - index [1]: Received CRC16 matches the calculated CRC16.
     *     - index [2]: Sequence flags are coherent with segmentation state.
     *     - index [3]: Sequence count matches the expected next count.
     * - Compare against template:
     *     - index [4]: Complete Packet Identification matches the template.
     *     - index [5]: Segmented/unsegmented class matches the template.
     */
    [[nodiscard]] std::vector<bool> getReport() const { return m_report; }

    /** @brief Clears the validator and resets sequence-stream state. */
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

    Packet m_templatePacket;               ///< Template packet used for validation.
    bool m_validatePacketCoherence{true};  ///< Whether to validate packet length, CRC, and sequence flags.
    bool m_validateAgainstTemplate{false}; ///< Whether to validate against the template packet.
    bool m_validateSegmentedCount{true};   ///< Whether to validate sequence-count continuity.
    std::uint16_t m_sequenceCounter{0};    ///< Expected count and segmentation state packed in one word.
    std::vector<bool> m_report{};          ///< Results of the performed validation checks.
    std::size_t m_reportSize{6};           ///< Expected report size.
    CRC16Config m_CRCConfig;
  };
} // namespace CCSDS

#endif // CCSDS_VALIDATOR_H
