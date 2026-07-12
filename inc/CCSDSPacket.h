// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_PACKET_H
#define CCSDS_PACKET_H

#include <CCSDSResult.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "CCSDSHeader.h"
#include "CCSDSDataField.h"

namespace CCSDS {
  struct CRC16Config {
    std::uint16_t polynomial = 0x1021;
    std::uint16_t initialValue = 0xFFFF;
    std::uint16_t finalXorValue = 0x0000;
  };

  enum class PacketErrorControlMode : std::uint8_t {
    None = 0,
    CRC16 = 1
  };

  class Packet {
  public:
    Packet() = default;

    ResultBool setPrimaryHeader(PrimaryHeader data);
    [[nodiscard]] ResultBool setPrimaryHeader(std::uint64_t data);
    [[nodiscard]] ResultBool setPrimaryHeader(const std::vector<std::uint8_t> &data);
    void setPrimaryHeader(const Header &header);

    void setDataFieldHeader(const std::shared_ptr<SecondaryHeaderAbstract> &header);

    template <typename T>
    ResultBool RegisterSecondaryHeader() {
      FORWARD_RESULT(m_dataField.RegisterSecondaryHeader<T>());
      return true;
    }

    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &data,
                                                const std::string &headerType);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                std::size_t sizeData,
                                                const std::string &headerType);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &data);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                std::size_t sizeData);

    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &data);
    [[nodiscard]] ResultBool setApplicationData(const std::uint8_t *pData,
                                                std::size_t sizeData);

    void setSequenceFlags(ESequenceFlag flags);
    [[nodiscard]] ResultBool setSequenceCount(std::uint16_t count);
    void setDataFieldSize(std::uint16_t size);
    void setUpdatePacketEnable(bool enable);

    void setPacketErrorControlMode(PacketErrorControlMode mode);
    [[nodiscard]] PacketErrorControlMode getPacketErrorControlMode() const {
      return (m_sequenceCounter & PACKET_ERROR_CONTROL_DISABLED_MASK) != 0U
               ? PacketErrorControlMode::None
               : PacketErrorControlMode::CRC16;
    }
    [[nodiscard]] std::uint16_t getPacketErrorControlSize() const {
      return getPacketErrorControlMode() == PacketErrorControlMode::CRC16 ? 2U : 0U;
    }

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

    std::uint64_t getPrimaryHeader64bit();
    std::uint16_t getFullPacketLength();
    std::vector<std::uint8_t> serialize();
    std::vector<std::uint8_t> getPrimaryHeaderBytes();
    std::vector<std::uint8_t> getDataFieldHeaderBytes();
    std::vector<std::uint8_t> getApplicationDataBytes();
    std::vector<std::uint8_t> getFullDataFieldBytes();
    std::vector<std::uint8_t> getCRCVectorBytes();
    std::uint16_t getCRC();
    std::uint16_t getDataFieldMaximumSize() const;
    bool getDataFieldHeaderFlag();
    DataField &getDataField();
    Header &getPrimaryHeader();

    void setCrcConfig(const CRC16Config crcConfig) { m_CRC16Config = crcConfig; }
    void setCrcConfig(const std::uint16_t polynomial,
                      const std::uint16_t initialValue,
                      const std::uint16_t finalXorValue) {
      m_CRC16Config = {polynomial, initialValue, finalXorValue};
    }

    void update();
    ResultBool loadFromConfigFile(const std::string &configPath);
#ifndef CCSDS_MCU
    ResultBool loadFromConfig(const Config &cfg);
#endif

  private:
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
