// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSSecondaryHeaderAbstract.h
 * @brief Defines the extension interface and raw-buffer implementation for packet secondary headers.
 */
#ifndef CCSDS_SECONDARY_HEADER_ABSTRACT_H
#define CCSDS_SECONDARY_HEADER_ABSTRACT_H

#include <CCSDSResult.h>
#include <vector>
#include <cstdint>

#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif

namespace CCSDS {
  class DataField;

  /**
   * @class SecondaryHeaderAbstract
   * @brief Interface implemented by optional packet data-field secondary headers.
   *
   * A secondary-header implementation owns only its own bytes and metadata. Packet
   * and DataField manage placement, primary-header flag synchronization, capacity,
   * and application data. getType() must return a stable registry key.
   *
   * update() is an explicit finalization callback. It is invoked by DataField::update()
   * or DataField::serialize() when automatic updates are enabled; ordinary getters do
   * not call it. Implementations may use the owning DataField to derive fields such as
   * application-data length.
   *
   * @par Variable-length headers
   * Set variableLength to true when deserialize() accepts sizes that cannot be known
   * from a default-constructed instance. Callers must then provide an explicit header
   * boundary when parsing ambiguous packet data fields.
   *
   * @code{.cpp}
   * class MissionHeader final : public CCSDS::SecondaryHeaderAbstract {
   * public:
   *   MissionHeader() { variableLength = true; }
   *   CCSDS::ResultBool deserialize(const std::vector<std::uint8_t>& bytes) override;
   *   void update(CCSDS::DataField* field) override;
   *   std::uint16_t getSize() const override;
   *   std::vector<std::uint8_t> serialize() const override;
   *   std::string getType() const override { return "MissionHeader"; }
   * };
   * @endcode
   */
  class SecondaryHeaderAbstract {
  public:
    /** @brief Enables safe destruction through a base-class pointer. */
    virtual ~SecondaryHeaderAbstract() = default;

    /**
     * @brief Replaces the header state from serialized secondary-header bytes.
     * @param data Bytes belonging only to this secondary header.
     * @return Success, or INVALID_SECONDARY_HEADER_DATA/type-specific error.
     * @note Implementations should reject invalid sizes and avoid partial mutation on failure.
     */
    [[nodiscard]] virtual ResultBool deserialize(const std::vector<std::uint8_t> &data) = 0;

    /**
     * @brief Refreshes derived header fields from the owning DataField.
     * @param dataField Non-owning pointer to the containing data field.
     * @note Implementations must not retain the pointer after the call.
     */
    virtual void update(DataField* dataField) = 0;

    /**
     * @brief Returns the current serialized header size.
     * @return Number of bytes produced by serialize().
     */
    [[nodiscard]] virtual std::uint16_t getSize() const = 0;

    /**
     * @brief Serializes only this secondary header.
     * @return Header bytes, excluding primary header, application data, and packet error control.
     */
    [[nodiscard]] virtual std::vector<std::uint8_t> serialize() const = 0;

    /**
     * @brief Returns the stable type name used by SecondaryHeaderFactory.
     * @return Non-empty registry key unique within a DataField factory.
     */
    [[nodiscard]] virtual std::string getType() const = 0;

#ifndef CCSDS_MCU
    /**
     * @brief Loads type-specific fields from Config.
     * @param config Parsed configuration object.
     * @return Success, or a configuration/type-specific validation error.
     */
    virtual ResultBool loadFromConfig(const Config &config) = 0;
#endif

    /**
     * @brief Declares whether the header accepts variable serialized lengths.
     * @param bEnable True for variable-length parsing.
     */
    void setVariableLength(const bool bEnable){ variableLength = bEnable; }

    /**
     * True when the implementation accepts lengths not equal to default getSize().
     * Public for compatibility; prefer setVariableLength() when changing it.
     */
    bool variableLength{false};
  };

  /**
   * @class BufferHeader
   * @brief Opaque secondary header that stores bytes without interpreting them.
   *
   * BufferHeader is used by setDataFieldHeader(bytes) and explicit-size parsing
   * overloads. It is suitable when a project needs a secondary-header boundary but
   * no typed decoder. Its registered type name is "DataOnlyHeader".
   */
  class BufferHeader final : public SecondaryHeaderAbstract {
  public:
    /** @brief Constructs an empty raw secondary header. */
    BufferHeader() = default;

    /**
     * @brief Constructs a raw secondary header from bytes.
     * @param data Bytes copied into the header.
     */
    explicit BufferHeader(const std::vector<std::uint8_t>& data) : m_data(data) {}

    /**
     * @brief Replaces the stored raw bytes.
     * @param data Bytes to copy.
     * @return Success.
     */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override {
      m_data = data;
      return true;
    }

    /** @brief Returns the number of stored raw bytes. */
    [[nodiscard]] std::uint16_t getSize() const override {
      return static_cast<std::uint16_t>(m_data.size());
    }

    /** @brief Returns the factory key "DataOnlyHeader". */
    [[nodiscard]] std::string getType() const override { return m_type; }

    /** @brief Returns a copy of the stored raw bytes. */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override { return m_data; }

    /**
     * @brief Refreshes the internal compatibility length from the current byte count.
     * @param dataField Unused.
     */
    void update(DataField* dataField) override {
      (void)dataField;
      m_dataLength = static_cast<std::uint16_t>(m_data.size());
    }
#ifndef CCSDS_MCU
    /** @brief Accepts configuration without modifying raw bytes. */
    ResultBool loadFromConfig(const Config &config) override {
      (void)config;
      return true;
    }
#endif

  private:
    std::vector<std::uint8_t> m_data; ///< Opaque serialized header bytes.
    std::uint16_t m_dataLength = 0;   ///< Compatibility field refreshed by update().
    const std::string m_type = "DataOnlyHeader"; ///< Factory registration key.
  };
}

#endif // CCSDS_SECONDARY_HEADER_ABSTRACT_H
