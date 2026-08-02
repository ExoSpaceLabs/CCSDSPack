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
#include "PusServices.h"

namespace CCSDS {
  /**
   * @class DataField
   * @brief Owns the optional secondary header and application-data bytes of one packet.
   *
   * DataField stores packet data-field content only. It does not include the six-byte
   * primary header or optional packet error-control bytes. Capacity is shared between
   * secondary-header and application-data bytes.
   *
   * Built-in secondary-header registrations include BufferHeader and the legacy,
   * project-specific PusA/PusB/PusC formats. Custom types can be registered through
   * RegisterSecondaryHeader().
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
    DataField() {
      bool noError = true;
      ASSIGN_OR_PRINT(noError, m_secondaryHeaderFactory.registerType(std::make_shared<BufferHeader>()));
      ASSIGN_OR_PRINT(noError, m_secondaryHeaderFactory.registerType(std::make_shared<PusA>()));
      ASSIGN_OR_PRINT(noError, m_secondaryHeaderFactory.registerType(std::make_shared<PusB>()));
      ASSIGN_OR_PRINT(noError, m_secondaryHeaderFactory.registerType(std::make_shared<PusC>()));
      if (!noError) {
        std::printf("[CCSDS DataField] Unable to create data field: secondary-header registration failed.");
      }
    }

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
      FORWARD_RESULT(m_secondaryHeaderFactory.registerType(std::make_shared<T>()));
      return true;
    }

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
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                const std::size_t &sizeData);

    /**
     * @brief Deserializes a registered secondary-header type from a byte span.
     * @param pData Pointer to the first secondary-header byte.
     * @param sizeData Number of bytes belonging to the secondary header.
     * @param pType Registered getType() string.
     * @return Success, or a null-pointer, unknown-type, size, or deserialization error.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                const std::size_t &sizeData,
                                                const std::string &pType);

    /**
     * @brief Deserializes a registered secondary-header type from bytes.
     * @param data Bytes belonging only to the secondary header.
     * @param pType Registered getType() string.
     * @return Success, or an unknown-type, size, capacity, or deserialization error.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &data,
                                                const std::string &pType);

    /**
     * @brief Stores opaque secondary-header bytes using BufferHeader.
     * @param dataFieldHeader Header bytes to copy.
     * @return Success, or INVALID_SECONDARY_HEADER_DATA when capacity is exceeded.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &dataFieldHeader);
#ifndef CCSDS_MCU
    /**
     * @brief Creates and loads a registered secondary header from Config.
     * @param cfg Configuration containing secondary_header_type and type-specific fields.
     * @return Success, or a configuration/registration/header error.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const Config &cfg);
#endif

    /**
     * @brief Installs a secondary-header object directly.
     * @param header Shared instance, or nullptr to remove the secondary header.
     * @note The DataField shares ownership and marks the header dirty for future update().
     */
    void setDataFieldHeader(std::shared_ptr<SecondaryHeaderAbstract> header);

    /** @brief Returns mutable access to the per-DataField secondary-header registry. */
    SecondaryHeaderFactory &getDataFieldHeaderFactory() { return m_secondaryHeaderFactory; }
    /** @brief Returns read-only access to the per-DataField secondary-header registry. */
    [[nodiscard]] const SecondaryHeaderFactory &getDataFieldHeaderFactory() const {
      return m_secondaryHeaderFactory;
    }

    /**
     * @brief Returns a reference to the installed secondary header.
     * @warning Dereferencing is undefined when no header is installed; check getDataFieldHeaderFlag() first.
     */
    SecondaryHeaderAbstract &getDataFieldHeader() { return *m_secondaryHeader; }
    /**
     * @brief Returns a read-only reference to the installed secondary header.
     * @warning Dereferencing is undefined when no header is installed; check getDataFieldHeaderFlag() first.
     */
    [[nodiscard]] const SecondaryHeaderAbstract &getDataFieldHeader() const { return *m_secondaryHeader; }

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
    void setDataFieldHeaderAutoUpdateStatus(const bool enable) { m_enableDataFieldUpdate = enable; }

    /** @brief Returns configured total packet data-field content capacity. */
    [[nodiscard]] std::uint16_t getDataFieldAbsoluteBytesSize() const;

    /** @brief Returns the number of stored application-data bytes. */
    [[nodiscard]] std::uint16_t getApplicationDataBytesSize() const;

    /** @brief Returns stored secondary-header plus application-data bytes. */
    [[nodiscard]] std::uint16_t getDataFieldUsedBytesSize() const;

    /** @brief Returns remaining capacity after current secondary-header and application data. */
    [[nodiscard]] std::uint16_t getDataFieldAvailableBytesSize() const;

    /** @brief Returns currently stored secondary-header bytes without finalizing. */
    std::vector<std::uint8_t> getDataFieldHeaderBytes();
    /** @brief Const overload of getDataFieldHeaderBytes(). */
    [[nodiscard]] std::vector<std::uint8_t> getDataFieldHeaderBytes() const;

    /**
     * @brief Finalizes the secondary header and serializes the complete data-field content.
     * @return Secondary-header bytes followed by application-data bytes.
     */
    std::vector<std::uint8_t> serialize();

    /** @brief Returns a copy of application data without finalizing. */
    std::vector<std::uint8_t> getApplicationData();
    /** @brief Const overload of getApplicationData(). */
    [[nodiscard]] std::vector<std::uint8_t> getApplicationData() const;

    /** @brief Returns whether automatic secondary-header update is enabled. */
    [[nodiscard]] bool getDataFieldHeaderAutoUpdateStatus() const { return m_enableDataFieldUpdate; }

    /** @brief Returns true when a secondary-header object is installed. */
    [[nodiscard]] bool getDataFieldHeaderFlag() const { return m_secondaryHeader != nullptr; }

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
    std::vector<std::uint8_t> m_applicationData{};               ///< Application-data bytes only.
    std::string m_dataFieldHeaderType{};                         ///< Lookup name of the installed header type.
    std::uint16_t m_dataPacketSize{2024};                        ///< Shared header/data capacity in bytes.
    bool m_dataFieldHeaderUpdated{false};                        ///< True after the latest explicit update.
    bool m_enableDataFieldUpdate{true};                          ///< Controls secondary-header update callbacks.
  };
}

#endif // CCSDS_DATA_FIELD_H
