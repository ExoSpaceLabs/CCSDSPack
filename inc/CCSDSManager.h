// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSManager.h
 * @brief Defines the single-stream CCSDS packet Manager.
 */
#ifndef CCSDS_MANAGER_H
#define CCSDS_MANAGER_H

#include <utility>
#include "CCSDSPacket.h"
#include "CCSDSResult.h"
#include "CCSDSValidator.h"

namespace CCSDS {
  /**
   * @class Manager
   * @brief Generates, stores, parses, and validates one CCSDS packet-identifier stream.
   *
   * A Manager is bound to one complete Packet Identification value: version,
   * packet type, data-field-header flag, and APID. Sequence flags, sequence count,
   * and Packet Data Length may vary inside that stream. Applications handling
   * multiple identifiers should create one Manager per identifier or use Packet
   * objects directly.
   *
   * A template can be supplied explicitly or the Manager can bind to the first
   * externally added packet. Mixed-identifier loads are rejected transactionally,
   * so a failure does not partially append packets or change the existing binding.
   *
   * @par Generation
   * setApplicationData() divides the input into packets according to the configured
   * data-field capacity, assigns segmentation flags, and consumes one 14-bit
   * sequence count per generated packet. Automatic counting wraps from 16383 to 0.
   *
   * @code{.cpp}
   * CCSDS::Packet packetTemplate;
   * packetTemplate.setPrimaryHeader({0, 0, 0, 0x123,
   *                                  CCSDS::UNSEGMENTED, 0, 0});
   * CCSDS::Manager manager(packetTemplate);
   * manager.setDataFieldSize(1024);
   * manager.setApplicationData(payload);
   * const auto stream = manager.getPacketsBuffer();
   * @endcode
   */
  class Manager {
  public:
    /** @brief Constructs an unbound Manager with automatic update, validation, and counting enabled. */
    Manager() = default;

    /**
     * @brief Constructs and binds a Manager to a packet template.
     * @param packet Template copied into the Manager after finalization.
     *
     * The template's complete Packet Identification becomes immutable for packets
     * accepted by this Manager until clear() is called. Its sequence count becomes
     * the initial stream count.
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
     * @brief Sets the four-byte synchronization marker used around serialized packets.
     * @param syncPattern Marker written and expected in big-endian order.
     * @note Sync handling is disabled by default and is project framing, not part of the CCSDS Space Packet.
     */
    void setSyncPattern(std::uint32_t syncPattern);

    /** @brief Returns the currently configured synchronization marker. */
    [[nodiscard]] std::uint32_t getSyncPattern() const;

    /**
     * @brief Enables or disables synchronization-marker insertion and parsing.
     * @param enable True to prefix each packet and require the marker during load().
     */
    void setSyncPatternEnable(bool enable);

    /** @brief Returns whether synchronization-marker handling is enabled. */
    [[nodiscard]] bool getSyncPatternEnable() const;

    /**
     * @brief Finalizes and installs a packet template, binding the Manager identifier.
     * @param packet Template packet to copy.
     * @return Success, TEMPLATE_SET_FAILURE when a template already exists, or a header error.
     * @note Call clear() before replacing an existing template.
     */
    [[nodiscard]] ResultBool setPacketTemplate(Packet packet);

    /**
     * @brief Loads and installs a template from a CCSDSPack configuration file.
     * @param configPath Path to the configuration file.
     * @return Success, or a file, configuration, template, or header error.
     */
    [[nodiscard]] ResultBool loadTemplateConfigFile(const std::string &configPath);
#ifndef CCSDS_MCU
    /**
     * @brief Loads and installs a template from an already parsed Config object.
     * @param cfg Packet configuration.
     * @return Success, or a configuration, template, or header error.
     */
    [[nodiscard]] ResultBool loadTemplateConfig(const Config &cfg);
#endif

    /**
     * @brief Sets the maximum secondary-header plus application-data bytes per generated packet.
     * @param size Capacity applied to the stored template.
     */
    void setDataFieldSize(std::uint16_t size);

    /** @brief Returns the template's currently available packet data-field capacity. */
    [[nodiscard]] std::uint16_t getDataFieldSize() const;

    /**
     * @brief Generates one or more packets from application data.
     * @param data Non-empty application payload to segment.
     * @return Success, or an error when no valid template/capacity is available.
     *
     * Existing generated packets are replaced atomically. A single packet receives
     * UNSEGMENTED; multiple packets receive FIRST, CONTINUING, and LAST flags.
     */
    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &data);

    /**
     * @brief Enables or disables automatic packet finalization for stored/generated packets.
     * @param enable True to let serialization refresh dependent fields.
     */
    void setAutoUpdateEnable(bool enable);

    /**
     * @brief Enables or disables Manager validation before selected read/serialization operations.
     * @param enable True to apply the owned Validator.
     */
    void setAutoValidateEnable(bool enable);

    /**
     * @brief Enables or disables automatic sequence-count advancement.
     * @param enable True to increment modulo 16384 after each generated or accepted packet.
     *
     * In manual mode, every generated packet reuses the count configured through
     * setSequenceCount(). Segmentation flags still describe packet position.
     */
    void setAutoSequenceCountEnable(bool enable);

    /** @brief Returns whether automatic sequence-count advancement is enabled. */
    [[nodiscard]] bool getAutoSequenceCountEnable() const {
      return (m_sequenceCount & AUTO_SEQUENCE_DISABLED_MASK) == 0U;
    }

    /**
     * @brief Sets the next Manager stream sequence count.
     * @param count Value in the inclusive range 0..16383.
     * @return Success, or INVALID_HEADER_DATA when count exceeds 14 bits.
     */
    [[nodiscard]] ResultBool setSequenceCount(std::uint16_t count);

    /** @brief Returns the next 14-bit sequence count used by this Manager. */
    [[nodiscard]] std::uint16_t getSequenceCount() const {
      return m_sequenceCount & SEQUENCE_COUNT_MASK;
    }

    /**
     * @brief Finalizes a copy of the template and returns its serialized bytes.
     * @return Serialized template, or NO_DATA when serialization produces no bytes.
     * @note The stored template is not mutated by this inspection path.
     */
    ResultBuffer getPacketTemplate();

    /**
     * @brief Finalizes a copy of one stored packet and returns its serialized bytes.
     * @param index Zero-based packet index.
     * @return Packet bytes, or an index, validation, or serialization error.
     */
    ResultBuffer getPacketBufferAtIndex(std::uint16_t index);

    /**
     * @brief Serializes all stored packets into one sequential byte buffer.
     * @return Concatenated packet bytes, optionally prefixed by sync patterns.
     * @note Stored packets are copied before finalization; this getter does not mutate them.
     */
    [[nodiscard]] std::vector<std::uint8_t> getPacketsBuffer() const;

    /**
     * @brief Reassembles application data from all stored packets in order.
     * @return Concatenated application data, or NO_DATA/VALIDATION_FAILURE.
     */
    [[nodiscard]] ResultBuffer getApplicationDataBuffer();

    /**
     * @brief Returns application data from one stored packet.
     * @param index Zero-based packet index.
     * @return Application bytes, or INVALID_DATA when the index is out of range.
     */
    ResultBuffer getApplicationDataBufferAtIndex(std::uint16_t index);

    /** @brief Returns the number of currently stored packets. */
    [[nodiscard]] std::uint16_t getTotalPackets() const;

    /** @brief Returns whether automatic packet finalization is enabled. */
    [[nodiscard]] bool getAutoUpdateEnable() const { return m_updateEnable; }

    /** @brief Returns a copy of the stored packet template. */
    Packet getTemplate() { return m_templatePacket; }
    /** @brief Const overload returning a copy of the stored packet template. */
    [[nodiscard]] Packet getTemplate() const { return m_templatePacket; }

    /** @brief Returns a copy of all stored packets. */
    std::vector<Packet> getPackets();
    /** @brief Const overload returning a copy of all stored packets. */
    [[nodiscard]] std::vector<Packet> getPackets() const;

    /**
     * @brief Adds one packet after enforcing the Manager's complete Packet Identification.
     * @param packet Packet copied into storage on success.
     * @return Success, or a header/identifier/validation error.
     *
     * When no template or prior packet exists, the first accepted packet establishes
     * the Manager binding. Sequence flags and counts are not part of that binding.
     */
    [[nodiscard]] ResultBool addPacket(Packet packet);

    /**
     * @brief Parses and adds exactly one packet from a buffer.
     * @param packetBuffer Buffer containing one packet at its beginning.
     * @return Success, or a parsing, checksum, identifier, or validation error.
     * @note Parsing uses the PEC mode bound by the template/stream, defaulting to CRC16.
     */
    [[nodiscard]] ResultBool addPacketFromBuffer(const std::vector<std::uint8_t> &packetBuffer);

    /**
     * @brief Transactionally loads a packet collection.
     * @param packets Packets to append in order.
     * @return Success, or the first identifier/validation error without partial mutation.
     */
    [[nodiscard]] ResultBool load(const std::vector<Packet> &packets);

    /**
     * @brief Transactionally parses and loads concatenated packet bytes.
     * @param packetsBuffer One or more adjacent packets, optionally sync-prefixed.
     * @return Success, or the first framing, parsing, CRC, identifier, or validation error.
     */
    [[nodiscard]] ResultBool load(const std::vector<std::uint8_t> &packetsBuffer);

    /**
     * @brief Reads a binary file and transactionally loads its packet stream.
     * @param binaryFile Input file path.
     * @return Success, or a file/parsing/validation error.
     */
    [[nodiscard]] ResultBool read(const std::string &binaryFile);

    /**
     * @brief Serializes stored packets and writes them to a binary file.
     * @param binaryFile Output file path.
     * @return Success, or a serialization or file-write error.
     */
    [[nodiscard]] ResultBool write(const std::string &binaryFile) const;

    /**
     * @brief Loads and installs a template from a .bin or .cfg file.
     * @param filename Template file path.
     * @return Success, or an extension, file, parsing, configuration, or template error.
     */
    [[nodiscard]] ResultBool readTemplate(const std::string &filename);

    /**
     * @brief Clears packets, template, identifier binding, Validator state, and sequence count.
     * @note Automatic/manual sequence mode is preserved while the count resets to zero.
     */
    void clear();

    /**
     * @brief Clears stored packets and resets sequence/Validator state while retaining the template binding.
     * @note Automatic/manual sequence mode is preserved while the count resets to zero.
     */
    void clearPackets();

    /**
     * @brief Returns mutable access to the owned Validator.
     * @return Reference valid for the lifetime of this Manager.
     * @warning Reconfiguration directly changes subsequent Manager validation behavior.
     */
    Validator &getValidatorReference() { return m_validator; }

    /**
     * @brief Returns mutable access to the internal packet collection.
     * @return Reference valid until the Manager is destroyed or moved.
     * @warning Direct edits bypass identifier checks, sequence synchronization, and validation.
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

    Packet m_templatePacket{};         ///< Template used for generation and identifier binding.
    bool m_templateIsSet{false};       ///< True after a template has been installed.
    bool m_updateEnable{true};         ///< Enables finalization on packet copies before output.
    bool m_validateEnable{true};       ///< Enables use of the owned Validator.
    bool m_syncPattEnable{false};      ///< Enables project-specific sync framing.
    std::vector<Packet> m_packets;     ///< Packets belonging to the bound identifier stream.
    std::uint16_t m_sequenceCount{0};  ///< Count in low 14 bits; manual-mode flag in bit 15.

    Validator m_validator{};           ///< Stateful validator for this stream.
    std::uint32_t m_syncPattern{0x1ACFFC1D}; ///< Project-specific four-byte sync marker.
  };
} // namespace CCSDS

#endif // CCSDS_MANAGER_H
