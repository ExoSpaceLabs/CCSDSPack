// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSDataField.h
 * @brief Defines storage and secondary-header handling for a CCSDS packet data field.
 */
#ifndef CCSDS_DATA_FIELD_H
#define CCSDS_DATA_FIELD_H

#include <CCSDSResult.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include "CCSDSSecondaryHeaderAbstract.h"
#include "CCSDSSecondaryHeaderFactory.h"
#include "CCSDSMissionProfile.h"
#include "PusSecondaryHeaderFactory.h"

namespace CCSDS {
  /**
   * @class DataField
   * @brief Owns the optional secondary header and application-data bytes of one packet.
   *
   * DataField stores packet data-field content only. It does not include the six-byte
   * primary header or optional packet error-control bytes. Capacity is shared between
   * secondary-header and application-data bytes.
   *
   * Custom secondary headers use SecondaryHeaderFactory. Standards-defined PUS
   * headers use the separate, fixed PusSecondaryHeaderFactory.
   *
   * Inspection APIs do not call update(). serialize() is the explicit finalization
   * path: it optionally invokes the installed secondary header's update() method and
   * then concatenates secondary-header and application-data bytes.
   */
  class DataField {
  public:
    /**
     * @brief Constructs an empty data field and registers built-in secondary-header types.
     *
     * Registration failures are printed because the constructor cannot return ResultBool.
     */
    DataField() { (void)m_secondaryHeaderFactory.registerType<BufferHeader>(); }

    /** @brief Destroys the data field and releases shared secondary-header ownership. */
    ~DataField() = default;

    /**
     * @brief Registers a custom secondary-header type.
     * @tparam T Default-constructible class derived from SecondaryHeaderAbstract.
     * @return Success, or the factory registration error.
     * @note T::getType() is used as the lookup key.
     */
    template <typename T>
    ResultBool RegisterSecondaryHeader() {
      FORWARD_RESULT(m_secondaryHeaderFactory.registerType<T>());
      return true;
    }

    [[nodiscard]] ResultBool setMissionProfile(const MissionProfile &profile);
    [[nodiscard]] const MissionProfile &getMissionProfile() const { return m_missionProfile; }

    /**
     * @brief Replaces application data from a vector.
     * @param applicationData Bytes to copy; an empty vector clears the data.
     * @return Success, or INVALID_APPLICATION_DATA when capacity is exceeded.
     */
    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &applicationData);

    /**
     * @brief Replaces application data from a byte span.
     * @param pData Pointer to the first byte.
     * @param sizeData Number of bytes to copy; must be non-zero.
     * @return Success, or a null-pointer, empty-span, or capacity error.
     */
    [[nodiscard]] ResultBool setApplicationData(const std::uint8_t *pData,
                                                const std::size_t &sizeData);

    /**
     * @brief Stores an opaque raw secondary header from a byte span.
     * @param pData Pointer to the first secondary-header byte.
     * @param sizeData Number of bytes to copy; must be non-zero.
     * @return Success, or a null-pointer, empty-span, or capacity error.
     */
    [[nodiscard]] ResultBool setSecondaryHeader(const std::uint8_t *pData,
                                                const std::size_t &sizeData);

    /**
     * @brief Deserializes a registered secondary-header type from a byte span.
     * @param pData Pointer to the first secondary-header byte.
     * @param sizeData Number of bytes belonging to the secondary header.
     * @param pType Registered getType() string.
     * @return Success, or a null-pointer, unknown-type, size, or deserialization error.
     */
    [[nodiscard]] ResultBool setSecondaryHeader(const std::uint8_t *pData,
                                                const std::size_t &sizeData,
                                                const std::string &pType);

    /**
     * @brief Deserializes a registered secondary-header type from bytes.
     * @param data Bytes belonging only to the secondary header.
     * @param pType Registered getType() string.
     * @return Success, or an unknown-type, size, capacity, or deserialization error.
     */
    [[nodiscard]] ResultBool setSecondaryHeader(const std::vector<std::uint8_t> &data,
                                                const std::string &pType);

    /**
     * @brief Stores opaque secondary-header bytes using BufferHeader.
     * @param secondaryHeader Header bytes to copy.
     * @return Success, or INVALID_SECONDARY_HEADER_DATA when capacity is exceeded.
     */
    [[nodiscard]] ResultBool setSecondaryHeader(const std::vector<std::uint8_t> &secondaryHeader);
#ifndef CCSDS_MCU
    /**
     * @brief Creates and loads a registered secondary header from Config.
     * @param cfg Configuration containing secondary_header_type and type-specific fields.
     * @return Success, or a configuration/registration/header error.
     */
    [[nodiscard]] ResultBool setSecondaryHeader(const Config &cfg);
#endif

    /**
     * @brief Installs a secondary-header object directly.
     * @param header Shared instance, or nullptr to remove the secondary header.
     * @note The DataField shares ownership and marks the header dirty for future update().
     */
    [[nodiscard]] ResultBool setSecondaryHeader(std::shared_ptr<SecondaryHeaderAbstract> header);

    /** @brief Returns mutable access to the per-DataField secondary-header registry. */
    SecondaryHeaderFactory &getSecondaryHeaderFactory() { return m_secondaryHeaderFactory; }
    /** @brief Returns read-only access to the per-DataField secondary-header registry. */
    [[nodiscard]] const SecondaryHeaderFactory &getSecondaryHeaderFactory() const {
      return m_secondaryHeaderFactory;
    }

    /** @brief Returns read-only access to the fixed standards PUS codec registry. */
    [[nodiscard]] const PusSecondaryHeaderFactory &getPusSecondaryHeaderFactory() const {
      return m_pusSecondaryHeaderFactory;
    }

    /**
     * @brief Sets total capacity shared by secondary-header and application-data bytes.
     * @param value Capacity in bytes.
     * @note Existing content is not truncated when capacity is reduced.
     */
    void setDataPacketSize(const std::uint16_t &value);

    /**
     * @brief Enables or disables calls to SecondaryHeaderAbstract::update().
     * @param enable True to let DataField::update()/serialize() refresh the header.
     */
    void setSecondaryHeaderAutoUpdateStatus(const bool enable) {
      m_enableSecondaryHeaderUpdate = enable;
    }

    /** @brief Returns configured total packet data-field content capacity. */
    [[nodiscard]] std::uint16_t getDataFieldAbsoluteBytesSize() const;

    /** @brief Returns the number of stored application-data bytes. */
    [[nodiscard]] std::uint16_t getApplicationDataBytesSize() const;

    /** @brief Returns stored secondary-header plus application-data bytes. */
    [[nodiscard]] std::uint16_t getDataFieldUsedBytesSize() const;

    /** @brief Returns remaining capacity after current secondary-header and application data. */
    [[nodiscard]] std::uint16_t getDataFieldAvailableBytesSize() const;

    /** @brief Returns currently stored secondary-header bytes without finalizing. */
    std::vector<std::uint8_t> getSecondaryHeaderBytes();
    /** @brief Const overload of getSecondaryHeaderBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getSecondaryHeaderBytes() const;

    /**
     * @brief Finalizes the secondary header and serializes the complete data-field content.
     * @return Secondary-header bytes followed by application data, or a header/capacity error.
     */
    [[nodiscard]] ResultBuffer serialize();

    /** @brief Returns a copy of application data without finalizing. */
    std::vector<std::uint8_t> getApplicationData();
    /** @brief Const overload of getApplicationData(). */
    [[nodiscard]] std::vector<std::uint8_t> getApplicationData() const;

    /** @brief Returns whether automatic secondary-header update is enabled. */
    [[nodiscard]] bool getSecondaryHeaderAutoUpdateStatus() const {
      return m_enableSecondaryHeaderUpdate;
    }

    /** @brief Returns true when a secondary-header object is installed. */
    [[nodiscard]] bool getSecondaryHeaderFlag() const { return m_secondaryHeader != nullptr; }

    /** @brief Returns shared mutable ownership of the installed secondary header, or nullptr. */
    std::shared_ptr<SecondaryHeaderAbstract> getSecondaryHeader();
    /** @brief Returns shared read-only ownership of the installed secondary header, or nullptr. */
    [[nodiscard]] std::shared_ptr<const SecondaryHeaderAbstract> getSecondaryHeader() const;

    /**
     * @brief Refreshes the installed secondary header once when it is dirty and updates are enabled.
     * @note Getter calls do not invoke this method.
     */
    void update();

  private:
    friend class Packet;
    void clearContent();

    std::shared_ptr<SecondaryHeaderAbstract> m_secondaryHeader{}; ///< Installed optional secondary header.
    SecondaryHeaderFactory m_secondaryHeaderFactory;             ///< Registry used by typed header parsing.
    PusSecondaryHeaderFactory m_pusSecondaryHeaderFactory;       ///< Fixed standards PUS codec registry.
    MissionProfile m_missionProfile{};                           ///< Explicit generic/PUS tailoring.
    std::vector<std::uint8_t> m_applicationData{};               ///< Application-data bytes only.
    std::string m_secondaryHeaderType{};                         ///< Lookup name of the installed header type.
    std::uint16_t m_dataPacketSize{2024};                        ///< Shared header/data capacity in bytes.
    bool m_secondaryHeaderUpdated{false};                        ///< True after the latest explicit update.
    bool m_enableSecondaryHeaderUpdate{true};                    ///< Controls secondary-header update callbacks.
  };
}

#endif // CCSDS_DATA_FIELD_H
