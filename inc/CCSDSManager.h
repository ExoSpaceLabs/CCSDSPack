// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_MANAGER_H
#define CCSDS_MANAGER_H

#include <utility>
#include "CCSDSPacket.h"
#include "CCSDSResult.h"
#include "CCSDSValidator.h"

namespace CCSDS {
  /**
   * @class Manager
   * @brief Manages CCSDS packets and their templates.
   *
   * This class provides an interface for managing CCSDS packets, allowing
   * users to set a packet template, configure data field sizes, and retrieve
   * application data and packet instances.
   */
  class Manager {
  public:
    /** @brief Default constructor. */
    Manager() = default;

    /**
     * @brief Constructs a Manager with a given packet template.
     * @param packet The packet template to be used as a reference.
     */
    explicit Manager(Packet packet) {
      packet.update();
      m_templatePacket = std::move(packet);
      m_templateIsSet = true;
      m_sequenceCount = m_templatePacket.getPrimaryHeader().getSequenceCount() & SEQUENCE_COUNT_MASK;
      m_templatePacket.setUpdatePacketEnable(false);
      m_validator.setTemplatePacket(m_templatePacket);
      m_validator.configure(true, true, true);
    }

    /**
     * Set the sync pattern that indicates the start of a CCSDS packet.
     * Insertion is disabled by default; use setSyncPatternEnable to enable it.
     */
    void setSyncPattern(std::uint32_t syncPattern);

    /** @return The currently configured sync pattern. */
    [[nodiscard]] std::uint32_t getSyncPattern() const;

    /** @brief Enables sync-pattern insertion and parsing. */
    void setSyncPatternEnable(bool enable);

    /** @return Whether sync-pattern handling is enabled. */
    [[nodiscard]] bool getSyncPatternEnable() const;

    /**
     * @brief Sets a new packet template.
     * @param packet The new packet template to use.
     */
    [[nodiscard]] ResultBool setPacketTemplate(Packet packet);

    /** @brief Loads a template packet from a configuration file. */
    [[nodiscard]] ResultBool loadTemplateConfigFile(const std::string &configPath);
#ifndef CCSDS_MCU
    /** @brief Loads a template packet from a configuration object. */
    [[nodiscard]] ResultBool loadTemplateConfig(const Config &cfg);
#endif

    /** @brief Sets the size of the data field. */
    void setDataFieldSize(std::uint16_t size);

    /** @return The configured data-field size, including a secondary header. */
    [[nodiscard]] std::uint16_t getDataFieldSize() const;

    /**
     * @brief Segments and assigns application data using the packet template.
     * @return ResultBool indicating success or failure.
     */
    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &data);

    /** @brief Enables or disables automatic packet finalization. */
    void setAutoUpdateEnable(bool enable);

    /** @brief Enables or disables automatic packet validation. */
    void setAutoValidateEnable(bool enable);

    /**
     * @brief Enables or disables automatic sequence-count advancement.
     *
     * Automatic mode assigns the current count to each generated packet and
     * advances modulo 16384. Manual mode reuses the configured count.
     */
    void setAutoSequenceCountEnable(bool enable);

    /** @return Whether automatic sequence-count advancement is enabled. */
    [[nodiscard]] bool getAutoSequenceCountEnable() const {
      return (m_sequenceCount & AUTO_SEQUENCE_DISABLED_MASK) == 0U;
    }

    /** @brief Sets the Manager stream sequence count in the range 0..16383. */
    [[nodiscard]] ResultBool setSequenceCount(std::uint16_t count);

    /** @return The current 14-bit Manager stream sequence count. */
    [[nodiscard]] std::uint16_t getSequenceCount() const {
      return m_sequenceCount & SEQUENCE_COUNT_MASK;
    }

    /** @brief Retrieves the packet template in serialized form. */
    ResultBuffer getPacketTemplate();

    /** @brief Retrieves a serialized packet at the specified index. */
    ResultBuffer getPacketBufferAtIndex(std::uint16_t index);

    /** @brief Retrieves all stored packets as one sequential byte buffer. */
    [[nodiscard]] std::vector<std::uint8_t> getPacketsBuffer() const;

    /** @brief Retrieves reassembled application data from the packets. */
    [[nodiscard]] ResultBuffer getApplicationDataBuffer();

    /** @brief Retrieves application data from one packet. */
    ResultBuffer getApplicationDataBufferAtIndex(std::uint16_t index);

    /** @return The total number of stored packets. */
    [[nodiscard]] std::uint16_t getTotalPackets() const;

    /** @return True when automatic packet updates are enabled. */
    [[nodiscard]] bool getAutoUpdateEnable() const { return m_updateEnable; }

    /** @return A copy of the stored packet template. */
    Packet getTemplate() { return m_templatePacket; }
    [[nodiscard]] Packet getTemplate() const { return m_templatePacket; }

    /** @return A copy of all stored packets. */
    std::vector<Packet> getPackets();
    [[nodiscard]] std::vector<Packet> getPackets() const;

    /**
     * @brief Adds a packet when its complete Packet Identification matches the
     * Manager-bound stream identifier.
     */
    [[nodiscard]] ResultBool addPacket(Packet packet);

    /** @brief Parses and adds one packet from a byte buffer. */
    [[nodiscard]] ResultBool addPacketFromBuffer(const std::vector<std::uint8_t> &packetBuffer);

    /** @brief Transactionally loads a collection of packets. */
    [[nodiscard]] ResultBool load(const std::vector<Packet> &packets);

    /** @brief Transactionally loads one or more concatenated packets. */
    [[nodiscard]] ResultBool load(const std::vector<std::uint8_t> &packetsBuffer);

    /** @brief Loads packets from a binary file. */
    [[nodiscard]] ResultBool read(const std::string &binaryFile);

    /** @brief Writes stored packets to a binary file. */
    [[nodiscard]] ResultBool write(const std::string &binaryFile) const;

    /** @brief Loads a template packet from a binary or configuration file. */
    [[nodiscard]] ResultBool readTemplate(const std::string &filename);

    /** @brief Clears packets, template, identifier binding, and sequence state. */
    void clear();

    /** @brief Clears stored packets and resets the sequence counter to zero. */
    void clearPackets();

    /**
     * @brief Returns a reference to the Manager's Validator.
     * @note Changing this instance affects Manager validation.
     */
    Validator &getValidatorReference() { return m_validator; }

    /**
     * @brief Returns a reference to the packets vector.
     * @note Direct mutation can bypass Manager invariants.
     */
    std::vector<Packet> &getPacketsReference() { return m_packets; }

  private:
    static constexpr std::uint16_t SEQUENCE_COUNT_MASK{0x3FFFU};
    static constexpr std::uint16_t AUTO_SEQUENCE_DISABLED_MASK{0x8000U};

    [[nodiscard]] static std::uint16_t packetIdentifier(const Packet &packet);
    [[nodiscard]] bool hasIdentifierBinding() const;
    [[nodiscard]] std::uint16_t boundPacketIdentifier() const;
    [[nodiscard]] ResultBool validatePacketIdentifier(const Packet &packet) const;
    [[nodiscard]] PacketErrorControlMode boundPacketErrorControlMode() const;
    void advanceSequenceCount();
    void syncSequenceCountFromPacket(const Packet &packet);

    Packet m_templatePacket{};         ///< Template used for generating new packets.
    bool m_templateIsSet{false};       ///< Whether a template has been configured.
    bool m_updateEnable{true};         ///< Whether automatic packet finalization is enabled.
    bool m_validateEnable{true};       ///< Whether automatic validation is enabled.
    bool m_syncPattEnable{false};      ///< Whether sync-pattern handling is enabled.
    std::vector<Packet> m_packets;     ///< Collection of stored packets.
    std::uint16_t m_sequenceCount{0};  ///< Count in low 14 bits; manual-mode flag in bit 15.

    Validator m_validator{};
    std::uint32_t m_syncPattern{0x1ACFFC1D};
  };
} // namespace CCSDS

#endif // CCSDS_MANAGER_H
