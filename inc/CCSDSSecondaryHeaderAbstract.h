// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSSecondaryHeaderAbstract.h
 * @brief Defines the extension interface and raw-buffer implementation for packet secondary headers.
 */
#ifndef CCSDS_SECONDARY_HEADER_ABSTRACT_H
#define CCSDS_SECONDARY_HEADER_ABSTRACT_H

#include <CCSDSPacketTypes.h>
#include <CCSDSResult.h>
#include <vector>
#include <cstdint>
#include <string>

#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif

namespace ccsds {
  class DataField;

  /**
   * @class SecondaryHeaderAbstract
   * @brief Interface implemented by optional packet data-field secondary headers.
   *
   * A secondary-header implementation owns only its own bytes and metadata. Packet
   * and DataField manage placement, primary-header flag synchronization, capacity,
   * and application data. getType() must return a stable registry key.
   *
   * A directional custom header may override getDirection(). Returning
   * PacketDirection::Unspecified leaves the CCSDS primary-header Packet Type under
   * caller control. Standards-defined PUS TC/TM headers always report a concrete
   * intrinsic direction.
   */
  class SecondaryHeaderAbstract {
  public:
    virtual ~SecondaryHeaderAbstract() = default;

    [[nodiscard]] virtual ResultBool deserialize(const std::vector<std::uint8_t> &data) = 0;
    virtual void update(DataField* dataField) = 0;
    [[nodiscard]] virtual std::uint16_t getSize() const = 0;
    [[nodiscard]] virtual std::vector<std::uint8_t> serialize() const = 0;
    [[nodiscard]] virtual std::string getType() const = 0;

    /** @brief Returns a header-declared packet direction, or Unspecified for direction-neutral headers. */
    [[nodiscard]] virtual PacketDirection getDirection() const noexcept {
      return PacketDirection::Unspecified;
    }

    /** @brief Identifies standards-defined PUS codecs without requiring RTTI. */
    [[nodiscard]] virtual bool isPusHeader() const { return false; }

#ifndef CCSDS_MCU
    virtual ResultBool loadFromConfig(const ccsds::Config &config) = 0;
#endif

    void setVariableLength(const bool bEnable){ variableLength = bEnable; }
    bool variableLength{false};
  };

  /**
   * @class BufferHeader
   * @brief Opaque direction-neutral secondary header that stores bytes without interpreting them.
   */
  class BufferHeader final : public SecondaryHeaderAbstract {
  public:
    BufferHeader() = default;
    explicit BufferHeader(const std::vector<std::uint8_t>& data) : m_data(data) {}

    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }

    [[nodiscard]] std::uint16_t getSize() const override {
      return static_cast<std::uint16_t>(m_data.size());
    }

    [[nodiscard]] std::string getType() const override { return m_type; }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }

    void update(DataField* dataField) override {
      (void)dataField;
      m_dataLength = static_cast<std::uint16_t>(m_data.size());
    }
#ifndef CCSDS_MCU
    ResultBool loadFromConfig(const ccsds::Config &config) override {
      (void)config;
      return true;
    }
#endif

  private:
    std::vector<std::uint8_t> m_data;
    std::uint16_t m_dataLength = 0;
    const std::string m_type = "DataOnlyHeader";
  };
}

#endif // CCSDS_SECONDARY_HEADER_ABSTRACT_H
