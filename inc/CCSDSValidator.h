/// @file CCSDSValidator.h
/// @brief Defines the Validator class for CCSDS packet validation.
#ifndef CCSDS_VALIDATOR_H
#define CCSDS_VALIDATOR_H

#include "CCSDSPacket.h"

namespace CCSDS {
  /**
   * @brief Handles validation of CCSDS packets.
   *
   * The Validator class checks packet coherence and compares packets against a template.
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
    explicit Validator(const Packet &templatePacket) : m_templatePacket(templatePacket) {
    };

    /**
     * @brief Sets the template packet for validation.
     * @param templatePacket The new template packet.
     */
    void setTemplatePacket(const Packet &templatePacket) { m_templatePacket = templatePacket; }

    /**
     * @brief Configures validation options.
     * @param validatePacketCoherence Enables/disables packet coherence validation.
     * @param validateSequenceCount
     * @param validateAgainstTemplate Enables/disables comparison against the template.
     */
    void configure(bool validatePacketCoherence, bool validateSequenceCount, bool validateAgainstTemplate);

    /**
     * @brief Validates a given packet.
     * @param packet The packet to validate.
     * @return True if the packet passes validation, otherwise false.
     */
    bool validate(const Packet &packet);

    /**
     * @brief Returns a report of performed validation checks.
     *
     * - Packet Coherence:
     *     - index [0]: Data Field Length Header declared equals actual data field length
     *     - index [1]: CRC15 value declared equals calculated CRC16 of the data field
     *     - index [2]: Sequence Control flags and count coherence
     *     - index [3]: Sequence Control count coherence (incremental start from 1)
     * - Compare Against Template:
     *     - index [4]: Identification and Version in packet matches template
     *     - index [5]: Template Sequence Control match
     *
     * @return A vector of boolean results for each performed check.
     */
    [[nodiscard]] std::vector<bool> getReport() const { return m_report; }

    /**
     * @brief Clears the validator, resets counter
     *
     */
    void clear();

  private:
    Packet m_templatePacket;               ///< Template packet used for validation.
    bool m_validatePacketCoherence{true};  ///< Whether to validate packet length and CRC (default is true).
    bool m_validateAgainstTemplate{false}; ///< Whether to validate against the template packet (default is false).
    bool m_validateSegmentedCount{true};  ///< Whether to validate the count of segmented packets.
    uint16_t m_sequenceCounter{1};         ///< Counter for segmented Packets
    std::vector<bool> m_report{};          ///< List of boolean results representing performed checks.
    size_t m_reportSize{6};                ///< Expected size of the validation report.
    CRC16Config m_CRCConfig;
  };
} // namespace CCSDS

#endif // CCSDS_VALIDATOR_H
