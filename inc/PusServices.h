// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file PusServices.h
 * @brief Defines legacy project-specific secondary-header formats retained for v1 compatibility.
 *
 * PusA, PusB, and PusC are historical CCSDSPack formats. Their names do not claim
 * conformance with a specific ECSS Packet Utilization Standard edition. Standards-
 * oriented ECSS PUS support is outside the v1.2 generic Space Packet scope.
 */
#ifndef PUS_SERVICES_H
#define PUS_SERVICES_H

#include "CCSDSSecondaryHeaderAbstract.h"
#include "CCSDSSecondaryHeaderFactory.h"
#include "CCSDSResult.h"

#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif

/**
 * @class PusA
 * @brief Legacy fixed-size six-byte telemetry-oriented secondary header.
 *
 * The serialized form contains project-specific version, service type, service
 * subtype, source ID, and application-data length fields. update() derives the
 * stored data length from the owning CCSDS::DataField when automatic updates are
 * enabled.
 *
 * @warning This compatibility type is not an ECSS PUS conformance declaration.
 */
class PusA final : public CCSDS::SecondaryHeaderAbstract {
public:
  /** @brief Constructs a zero-initialized six-byte PusA header. */
  PusA() = default;

  /**
   * @brief Constructs a PusA header from explicit field values.
   * @param version Three-bit project header version; high bits are masked.
   * @param serviceType Eight-bit service type.
   * @param serviceSubtype Eight-bit service subtype.
   * @param sourceID Eight-bit source identifier.
   * @param dataLength Initial application-data length; update() may replace it.
   */
  explicit PusA(const std::uint8_t version, const std::uint8_t serviceType,
                const std::uint8_t serviceSubtype, const std::uint8_t sourceID,
                const std::uint32_t dataLength)
    : m_version(version & 0x7),
      m_serviceType(serviceType),
      m_serviceSubType(serviceSubtype),
      m_sourceID(sourceID),
      m_dataLength(dataLength) {}

  /** @brief Returns the stored three-bit project header version. */
  [[nodiscard]] std::uint8_t getVersion() const { return m_version; }
  /** @brief Returns the stored service type. */
  [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
  /** @brief Returns the stored service subtype. */
  [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubType; }
  /** @brief Returns the stored source identifier. */
  [[nodiscard]] std::uint8_t getSourceID() const { return m_sourceID; }
  /** @brief Returns the stored application-data length. */
  [[nodiscard]] std::uint16_t getDataLength() const { return m_dataLength; }
  /** @brief Returns the fixed serialized size of six bytes. */
  [[nodiscard]] std::uint16_t getSize() const override { return m_size; }
  /** @brief Returns the secondary-header factory key "PusA". */
  [[nodiscard]] std::string getType() const override { return m_type; }

  /** @brief Serializes the six-byte project-specific header. */
  [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

  /**
   * @brief Replaces fields from exactly one serialized PusA header.
   * @param data Header bytes.
   * @return Success, or INVALID_SECONDARY_HEADER_DATA for invalid input.
   */
  [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<std::uint8_t> &data) override;

  /**
   * @brief Refreshes data length from the owning DataField application-data size.
   * @param dataField Non-owning pointer valid only for the duration of the call.
   */
  void update(CCSDS::DataField* dataField) override;

#ifndef CCSDS_MCU
  /**
   * @brief Loads project-specific PusA fields from Config.
   * @param cfg Parsed configuration.
   * @return Success, or a configuration/type/range error.
   */
  CCSDS::ResultBool loadFromConfig(const ::Config &cfg) override;
#endif

private:
  std::uint8_t m_version{};        ///< Three-bit project header version.
  std::uint8_t m_serviceType{};    ///< Service type.
  std::uint8_t m_serviceSubType{}; ///< Service subtype.
  std::uint8_t m_sourceID{};       ///< Source identifier.
  std::uint16_t m_dataLength{};    ///< Application-data length maintained by update().

  const std::string m_type = "PusA"; ///< Factory registration key.
  const std::uint16_t m_size = 6;    ///< Serialized size in bytes.
};

/**
 * @class PusB
 * @brief Legacy fixed-size eight-byte event-oriented secondary header.
 *
 * PusB extends the PusA-style fields with a 16-bit event identifier. update()
 * derives the stored data length from the owning CCSDS::DataField.
 *
 * @warning This compatibility type is not an ECSS PUS conformance declaration.
 */
class PusB final : public CCSDS::SecondaryHeaderAbstract {
public:
  /** @brief Constructs a zero-initialized eight-byte PusB header. */
  PusB() = default;

  /**
   * @brief Constructs a PusB header from explicit field values.
   * @param version Three-bit project header version; high bits are masked.
   * @param serviceType Eight-bit service type.
   * @param serviceSubtype Eight-bit service subtype.
   * @param sourceID Eight-bit source identifier.
   * @param eventID Sixteen-bit event identifier.
   * @param dataLength Initial application-data length; update() may replace it.
   */
  explicit PusB(const std::uint8_t version, const std::uint8_t serviceType,
                const std::uint8_t serviceSubtype, const std::uint8_t sourceID,
                const std::uint16_t eventID, const std::uint16_t dataLength)
    : m_version(version & 0x7),
      m_serviceType(serviceType),
      m_serviceSubType(serviceSubtype),
      m_sourceID(sourceID),
      m_eventID(eventID),
      m_dataLength(dataLength) {}

  /** @brief Returns the stored three-bit project header version. */
  [[nodiscard]] std::uint8_t getVersion() const { return m_version; }
  /** @brief Returns the stored service type. */
  [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
  /** @brief Returns the stored service subtype. */
  [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubType; }
  /** @brief Returns the stored source identifier. */
  [[nodiscard]] std::uint8_t getSourceID() const { return m_sourceID; }
  /** @brief Returns the stored event identifier. */
  [[nodiscard]] std::uint16_t getEventID() const { return m_eventID; }
  /** @brief Returns the stored application-data length. */
  [[nodiscard]] std::uint16_t getDataLength() const { return m_dataLength; }
  /** @brief Returns the fixed serialized size of eight bytes. */
  [[nodiscard]] std::uint16_t getSize() const override { return m_size; }
  /** @brief Returns the secondary-header factory key "PusB". */
  [[nodiscard]] std::string getType() const override { return m_type; }

  /** @brief Serializes the eight-byte project-specific header. */
  [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

  /**
   * @brief Replaces fields from exactly one serialized PusB header.
   * @param data Header bytes.
   * @return Success, or INVALID_SECONDARY_HEADER_DATA for invalid input.
   */
  [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<std::uint8_t> &data) override;

  /**
   * @brief Refreshes data length from the owning DataField application-data size.
   * @param dataField Non-owning pointer valid only for the duration of the call.
   */
  void update(CCSDS::DataField* dataField) override;

#ifndef CCSDS_MCU
  /**
   * @brief Loads project-specific PusB fields from Config.
   * @param cfg Parsed configuration.
   * @return Success, or a configuration/type/range error.
   */
  CCSDS::ResultBool loadFromConfig(const ::Config &cfg) override;
#endif

private:
  std::uint8_t m_version{};        ///< Three-bit project header version.
  std::uint8_t m_serviceType{};    ///< Service type.
  std::uint8_t m_serviceSubType{}; ///< Service subtype.
  std::uint8_t m_sourceID{};       ///< Source identifier.
  std::uint16_t m_eventID{};       ///< Event identifier.
  std::uint16_t m_dataLength{};    ///< Application-data length maintained by update().

  const std::string m_type = "PusB"; ///< Factory registration key.
  const std::uint16_t m_size = 8;    ///< Serialized size in bytes.
};

/**
 * @class PusC
 * @brief Legacy variable-length time-code secondary header.
 *
 * PusC stores project-specific version/service/source fields, a variable byte-vector
 * time code, and application-data length. Because the time-code size is not encoded
 * independently by the generic packet parser, callers should provide an explicit
 * secondary-header boundary when decoding from a packet byte stream.
 *
 * @warning This compatibility type is not an ECSS PUS conformance declaration.
 */
class PusC final : public CCSDS::SecondaryHeaderAbstract {
public:
  /** @brief Constructs an empty variable-length PusC header. */
  PusC() { variableLength = true; }

  /**
   * @brief Constructs a PusC header from explicit field values.
   * @param version Three-bit project header version; high bits are masked.
   * @param serviceType Eight-bit service type.
   * @param serviceSubtype Eight-bit service subtype.
   * @param sourceID Eight-bit source identifier.
   * @param timeCode Variable-length time-code bytes.
   * @param dataLength Initial application-data length; update() may replace it.
   */
  explicit PusC(const std::uint8_t version, const std::uint8_t serviceType,
                const std::uint8_t serviceSubtype, const std::uint8_t sourceID,
                const std::vector<std::uint8_t>& timeCode,
                const std::uint16_t dataLength)
    : m_version(version & 0x7),
      m_serviceType(serviceType),
      m_serviceSubType(serviceSubtype),
      m_sourceID(sourceID),
      m_timeCode(timeCode),
      m_dataLength(dataLength) {
    variableLength = true;
  }

  /** @brief Returns the stored three-bit project header version. */
  [[nodiscard]] std::uint8_t getVersion() const { return m_version; }
  /** @brief Returns the stored service type. */
  [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
  /** @brief Returns the stored service subtype. */
  [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubType; }
  /** @brief Returns the stored source identifier. */
  [[nodiscard]] std::uint8_t getSourceID() const { return m_sourceID; }
  /** @brief Returns a copy of the variable-length time-code bytes. */
  [[nodiscard]] std::vector<std::uint8_t> getTimeCode() const { return m_timeCode; }
  /** @brief Returns the stored application-data length. */
  [[nodiscard]] std::uint16_t getDataLength() const { return m_dataLength; }
  /** @brief Returns six fixed bytes plus the current time-code byte count. */
  [[nodiscard]] std::uint16_t getSize() const override {
    return static_cast<std::uint16_t>(m_size + m_timeCode.size());
  }
  /** @brief Returns the secondary-header factory key "PusC". */
  [[nodiscard]] std::string getType() const override { return m_type; }

  /** @brief Serializes fixed fields, time-code bytes, and data length. */
  [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

  /**
   * @brief Replaces fields from one explicitly bounded serialized PusC header.
   * @param data Header bytes including the variable-length time-code portion.
   * @return Success, or INVALID_SECONDARY_HEADER_DATA for invalid input.
   */
  [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<std::uint8_t> &data) override;

  /**
   * @brief Refreshes data length from the owning DataField application-data size.
   * @param dataField Non-owning pointer valid only for the duration of the call.
   */
  void update(CCSDS::DataField* dataField) override;

#ifndef CCSDS_MCU
  /**
   * @brief Loads project-specific PusC fields and time-code bytes from Config.
   * @param cfg Parsed configuration.
   * @return Success, or a configuration/type/range error.
   */
  CCSDS::ResultBool loadFromConfig(const ::Config &cfg) override;
#endif

private:
  std::uint8_t m_version{};               ///< Three-bit project header version.
  std::uint8_t m_serviceType{};           ///< Service type.
  std::uint8_t m_serviceSubType{};        ///< Service subtype.
  std::uint8_t m_sourceID{};              ///< Source identifier.
  std::vector<std::uint8_t> m_timeCode{}; ///< Variable-length time-code bytes.
  std::uint16_t m_dataLength{};           ///< Application-data length maintained by update().

  const std::string m_type = "PusC"; ///< Factory registration key.
  const std::uint16_t m_size = 6;    ///< Fixed serialized bytes excluding the time code.
};

#endif // PUS_SERVICES_H
