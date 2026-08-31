// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_MANAGER_H
#define CCSDS_MANAGER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "CCSDSPacket.h"
#include "CCSDSResult.h"
#include "CCSDSValidator.h"

namespace ccsds {

  /**
   * @brief Generates, stores, parses, and validates one CCSDS packet-identifier stream.
   *
   * The complete Packet template is the receive/generation contract. If the template
   * contains a PUS/custom secondary-header object, that object is also the parsing
   * prototype and carries any PUS tailoring. Manager owns no independent profile.
   */
  class Manager {
  public:
    Manager() = default;

    explicit Manager(Packet packet) {
      m_templatePacket = std::move(packet);
      m_templateIsSet = true;
      m_sequenceCount = m_templatePacket.getPrimaryHeader().getSequenceCount() & SEQUENCE_COUNT_MASK;
      m_templatePacket.setUpdatePacketEnable(false);
      m_validator.setTemplatePacket(m_templatePacket);
      m_validator.configure(true, true, true);
    }

    void setSyncPattern(std::uint32_t syncPattern);
    [[nodiscard]] std::uint32_t getSyncPattern() const;
    void setSyncPatternEnable(bool enable);
    [[nodiscard]] bool getSyncPatternEnable() const;

    [[nodiscard]] ResultBool setPacketTemplate(Packet packet);
    [[nodiscard]] ResultBool loadTemplateConfigFile(const std::string &configPath);
#ifndef CCSDS_MCU
    [[nodiscard]] ResultBool loadTemplateConfig(const ccsds::Config &cfg);
#endif

    void setDataFieldSize(std::uint16_t size);
    [[nodiscard]] std::uint16_t getDataFieldSize() const;

    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &data);
    [[nodiscard]] ResultBool setApplicationData(const std::uint8_t *data,
                                                const std::size_t size) {
      RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                     "Cannot set Application data, raw buffer pointer is null");
      RET_IF_ERR_MSG(size == 0U, ErrorCode::NO_DATA,
                     "Cannot set Application data, raw buffer is empty");
      return setApplicationData(std::vector<std::uint8_t>(data, data + size));
    }

    void setAutoUpdateEnable(bool enable);
    void setAutoValidateEnable(bool enable);
    void setAutoSequenceCountEnable(bool enable);
    [[nodiscard]] bool getAutoSequenceCountEnable() const {
      return (m_sequenceCount & AUTO_SEQUENCE_DISABLED_MASK) == 0U;
    }
    [[nodiscard]] ResultBool setSequenceCount(std::uint16_t count);
    [[nodiscard]] std::uint16_t getSequenceCount() const {
      return m_sequenceCount & SEQUENCE_COUNT_MASK;
    }

    [[nodiscard]] ResultBuffer getPacketTemplate();
    [[nodiscard]] ResultBuffer getPacketBufferAtIndex(std::uint16_t index);
    [[nodiscard]] ResultBuffer getPacketsBuffer() const;
    [[nodiscard]] ResultBuffer getApplicationDataBuffer();
    ResultBuffer getApplicationDataBufferAtIndex(std::uint16_t index);
    [[nodiscard]] std::uint16_t getTotalPackets() const;
    [[nodiscard]] bool getAutoUpdateEnable() const { return m_updateEnable; }

    Packet getTemplate() { return m_templatePacket; }
    [[nodiscard]] Packet getTemplate() const { return m_templatePacket; }
    [[nodiscard]] const Packet &getTemplateReference() const noexcept { return m_templatePacket; }

    std::vector<Packet> getPackets();
    [[nodiscard]] std::vector<Packet> getPackets() const;

    [[nodiscard]] ResultBool addPacket(Packet packet);
    [[nodiscard]] ResultBool addPacketFromBuffer(const std::vector<std::uint8_t> &packetBuffer);
    [[nodiscard]] ResultBool addPacketFromBuffer(const std::uint8_t *data,
                                                 const std::size_t size) {
      RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                     "Cannot add packet, raw buffer pointer is null");
      RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                     "Cannot add packet, raw buffer is empty");
      return addPacketFromBuffer(std::vector<std::uint8_t>(data, data + size));
    }

    [[nodiscard]] ResultBool load(const std::vector<Packet> &packets);
    [[nodiscard]] ResultBool load(const std::vector<std::uint8_t> &packetsBuffer);
    [[nodiscard]] ResultBool load(const std::uint8_t *data, const std::size_t size) {
      RET_IF_ERR_MSG(data == nullptr, ErrorCode::NULL_POINTER,
                     "Cannot load packet stream, raw buffer pointer is null");
      RET_IF_ERR_MSG(size == 0U, ErrorCode::INVALID_DATA,
                     "Cannot load packet stream, raw buffer is empty");
      return load(std::vector<std::uint8_t>(data, data + size));
    }

    [[nodiscard]] ResultBool read(const std::string &binaryFile);
    [[nodiscard]] ResultBool write(const std::string &binaryFile) const;
    [[nodiscard]] ResultBool readTemplate(const std::string &filename);

    void clear();
    void clearPackets();

    Validator &getValidatorReference() { return m_validator; }
    [[nodiscard]] const Validator &getValidatorReference() const noexcept { return m_validator; }

    std::vector<Packet> &getPacketsReference() { return m_packets; }
    [[nodiscard]] const std::vector<Packet> &getPacketsReference() const noexcept { return m_packets; }

  private:
    static constexpr std::uint16_t SEQUENCE_COUNT_MASK{0x3FFFU};
    static constexpr std::uint16_t AUTO_SEQUENCE_DISABLED_MASK{0x8000U};

    [[nodiscard]] static std::uint16_t packetIdentifier(const Packet &packet);
    [[nodiscard]] bool hasIdentifierBinding() const;
    [[nodiscard]] std::uint16_t boundPacketIdentifier() const;
    [[nodiscard]] ResultBool validatePacketIdentifier(const Packet &packet) const;
    [[nodiscard]] PacketErrorControlMode boundPacketErrorControlMode() const;
    [[nodiscard]] Packet boundParserPacket() const;
    void advanceSequenceCount();
    void syncSequenceCountFromPacket(const Packet &packet);

    Packet m_templatePacket{};
    bool m_templateIsSet{false};
    bool m_updateEnable{true};
    bool m_validateEnable{true};
    bool m_syncPattEnable{false};
    std::vector<Packet> m_packets;
    std::uint16_t m_sequenceCount{0};
    Validator m_validator{};
    std::uint32_t m_syncPattern{0x1ACFFC1D};
  };
} // namespace ccsds

#endif // CCSDS_MANAGER_H
