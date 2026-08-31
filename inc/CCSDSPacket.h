// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSPacket.h
 * @brief CCSDS 133.0-B-2 Space Packet container, parsing, and packet-level error control.
 */
#ifndef CCSDS_PACKET_H
#define CCSDS_PACKET_H

#include <CCSDSResult.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "CCSDSHeader.h"
#include "CCSDSDataField.h"
#include "CCSDSPacketTypes.h"

namespace ccsds {

  /** @brief Parameters for the optional CCSDSPack packet-level CRC16 trailer. */
  struct CRC16Config {
    std::uint16_t polynomial = 0x1021;
    std::uint16_t initialValue = 0xFFFF;
    std::uint16_t finalXorValue = 0x0000;
  };

  /**
   * @class Packet
   * @brief Owns, serializes, and parses one CCSDS Space Packet PDU.
   *
   * Packet-level policy such as CRC16 remains independent of secondary-header type.
   * Directional secondary headers may report PacketDirection through
   * SecondaryHeaderAbstract::getDirection(); installing one synchronizes the CCSDS
   * primary-header Packet Type. PUS revision/direction and PUS layout tailoring are
   * owned by the concrete PUS secondary-header object rather than by Packet.
   */
  class Packet {
  public:
    Packet() = default;

    ResultBool setPrimaryHeader(PrimaryHeader data);
    [[nodiscard]] ResultBool setPrimaryHeader(std::uint64_t data);
    [[nodiscard]] ResultBool setPrimaryHeader(const std::vector<std::uint8_t> &data);
    void setPrimaryHeader(const Header &header);

    /**
     * @brief Installs a secondary header and synchronizes flag/direction metadata.
     * @note A concrete header direction, when provided, becomes the CCSDS Packet Type.
     */
    [[nodiscard]] ResultBool setSecondaryHeader(
      const std::shared_ptr<SecondaryHeaderAbstract> &header);

    template <typename T>
    ResultBool RegisterSecondaryHeader() {
      FORWARD_RESULT(m_dataField.RegisterSecondaryHeader<T>());
      return true;
    }

    [[nodiscard]] ResultBool setSecondaryHeader(const std::vector<std::uint8_t> &data,
                                                const std::string &headerType);
    [[nodiscard]] ResultBool setSecondaryHeader(const std::uint8_t *pData,
                                                std::size_t sizeData,
                                                const std::string &headerType);
    [[nodiscard]] ResultBool setSecondaryHeader(const std::vector<std::uint8_t> &data);
    [[nodiscard]] ResultBool setSecondaryHeader(const std::uint8_t *pData,
                                                std::size_t sizeData);

    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &data);
    [[nodiscard]] ResultBool setApplicationData(const std::uint8_t *pData,
                                                std::size_t sizeData);

    void setSequenceFlags(ESequenceFlag flags);
    [[nodiscard]] ResultBool setSequenceCount(std::uint16_t count);
    void setDataFieldSize(std::uint16_t size);
    void setUpdatePacketEnable(bool enable);

    /** @brief Selects packet-level error control independently of PUS/custom headers. */
    void setPacketErrorControlMode(PacketErrorControlMode mode);
    [[nodiscard]] PacketErrorControlMode getPacketErrorControlMode() const {
      return (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK) != 0U
               ? PacketErrorControlMode::None
               : PacketErrorControlMode::CRC16;
    }
    [[nodiscard]] std::uint16_t getPacketErrorControlSize() const {
      return getPacketErrorControlMode() == PacketErrorControlMode::CRC16 ? 2U : 0U;
    }

    /** @brief Returns direction encoded by the CCSDS primary-header Packet Type. */
    [[nodiscard]] PacketDirection getDirection() const noexcept {
      return directionForPacketType(m_primaryHeader.getType());
    }

    /**
     * @brief Parses one packet. If a PUS header is already installed it is used as
     * the parsing prototype, including any optional tailoring.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data);

    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data,
                                         const std::string &headerType,
                                         std::int32_t headerSize = -1);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data,
                                         std::uint16_t headerDataSizeBytes);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &headerData,
                                         const std::vector<std::uint8_t> &data);

    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data);
    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data,
                                                         const std::string &headerType,
                                                         std::int32_t headerSize = -1);
    [[nodiscard]] Result<std::size_t> deserializeBounded(const std::vector<std::uint8_t> &data,
                                                         std::uint16_t headerDataSizeBytes);

    /**
     * @brief Typed parse convenience using a default-constructed or explicitly
     * tailored secondary-header prototype.
     *
     * Example: `packet.deserialize<ccsds::pus::rev_c::TmHeader>(wire)`.
     */
    template <typename HeaderT, typename... Args>
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data,
                                         Args&&... args) {
      const auto result = deserializeBounded<HeaderT>(data, std::forward<Args>(args)...);
      if (!result) return result.error();
      return true;
    }

    /** @brief Typed bounded parse returning the number of consumed bytes. */
    template <typename HeaderT, typename... Args>
    [[nodiscard]] Result<std::size_t> deserializeBounded(
        const std::vector<std::uint8_t> &data, Args&&... args) {
      static_assert(std::is_base_of<SecondaryHeaderAbstract, HeaderT>::value,
                    "HeaderT must derive from ccsds::SecondaryHeaderAbstract");
      Packet staged = *this;
      auto header = std::make_shared<HeaderT>(std::forward<Args>(args)...);
      const auto attach = staged.setSecondaryHeader(header);
      if (!attach) return attach.error();
      const auto parsed = staged.deserializeBounded(data);
      if (!parsed) return parsed.error();
      *this = std::move(staged);
      return parsed.value();
    }

    std::uint64_t getPrimaryHeader64bit();
    [[nodiscard]] std::uint64_t getPrimaryHeader64bit() const;
    std::uint16_t getFullPacketLength();
    [[nodiscard]] std::uint16_t getFullPacketLength() const;
    [[nodiscard]] std::size_t getSerializedSize() const {
      return 6U + static_cast<std::size_t>(m_dataField.getDataFieldUsedBytesSize())
             + static_cast<std::size_t>(getPacketErrorControlSize());
    }

    [[nodiscard]] ResultBuffer serialize();

    std::vector<std::uint8_t> getPrimaryHeaderBytes();
    [[nodiscard]] std::vector<std::uint8_t> getPrimaryHeaderBytes() const;
    std::shared_ptr<SecondaryHeaderAbstract> getSecondaryHeader();
    [[nodiscard]] std::shared_ptr<const SecondaryHeaderAbstract> getSecondaryHeader() const;
    std::vector<std::uint8_t> getSecondaryHeaderBytes();
    [[nodiscard]] std::vector<std::uint8_t> getSecondaryHeaderBytes() const;
    std::vector<std::uint8_t> getApplicationDataBytes();
    [[nodiscard]] std::vector<std::uint8_t> getApplicationDataBytes() const;
    std::vector<std::uint8_t> getFullDataFieldBytes();
    [[nodiscard]] std::vector<std::uint8_t> getFullDataFieldBytes() const;
    std::vector<std::uint8_t> getCRCVectorBytes();
    [[nodiscard]] std::vector<std::uint8_t> getCRCVectorBytes() const;
    std::uint16_t getCRC();
    [[nodiscard]] std::uint16_t getCRC() const;
    [[nodiscard]] std::uint16_t getDataFieldMaximumSize() const;
    bool getSecondaryHeaderFlag();
    [[nodiscard]] bool getSecondaryHeaderFlag() const;

    DataField &getDataField();
    [[nodiscard]] const DataField &getDataField() const;
    Header &getPrimaryHeader();
    [[nodiscard]] const Header &getPrimaryHeader() const;

    void setCrcConfig(const CRC16Config crcConfig) {
      m_CRC16Config = crcConfig;
      m_updateStatus = false;
    }
    void setCrcConfig(const std::uint16_t polynomial,
                      const std::uint16_t initialValue,
                      const std::uint16_t finalXorValue) {
      m_CRC16Config = {polynomial, initialValue, finalXorValue};
      m_updateStatus = false;
    }

    [[nodiscard]] ResultBool update();
    ResultBool loadFromConfigFile(const std::string &configPath);
#ifndef CCSDS_MCU
    ResultBool loadFromConfig(const ccsds::Config &cfg);
#endif

  private:
    [[nodiscard]] Result<std::size_t> deserializeBoundedWithSecondaryHeader(
      const std::vector<std::uint8_t> &data,
      const std::shared_ptr<SecondaryHeaderAbstract> &prototype,
      std::int32_t customHeaderSize = -1);

    static constexpr std::uint16_t SEQUENCE_COUNT_MASK = 0x3FFFU;
    static constexpr std::uint16_t PACKET_ERROR_CONTROL_DISABLED_MASK = 0x8000U;

    Header m_primaryHeader{};
    DataField m_dataField{};
    std::uint16_t m_CRC16{};
    CRC16Config m_CRC16Config;
    bool m_updateStatus{false};
    bool m_enableUpdatePacket{true};
    std::uint16_t m_sequenceCounter{0};
  };
}

#endif // CCSDS_PACKET_H
