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
#include "PusSecondaryHeaderFactory.h"

namespace ccsds {
  class DataField {
  public:
    DataField() { (void)m_secondaryHeaderFactory.registerType<BufferHeader>(); }
    ~DataField() = default;

    template <typename T>
    ResultBool RegisterSecondaryHeader() {
      FORWARD_RESULT(m_secondaryHeaderFactory.registerType<T>());
      return true;
    }

    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &applicationData);
    [[nodiscard]] ResultBool setApplicationData(const std::uint8_t *pData,
                                                const std::size_t &sizeData);

    [[nodiscard]] ResultBool setSecondaryHeader(const std::uint8_t *pData,
                                                const std::size_t &sizeData);
    [[nodiscard]] ResultBool setSecondaryHeader(const std::uint8_t *pData,
                                                const std::size_t &sizeData,
                                                const std::string &pType);
    [[nodiscard]] ResultBool setSecondaryHeader(const std::vector<std::uint8_t> &data,
                                                const std::string &pType);
    [[nodiscard]] ResultBool setSecondaryHeader(const std::vector<std::uint8_t> &secondaryHeader);
#ifndef CCSDS_MCU
    [[nodiscard]] ResultBool setSecondaryHeader(const ccsds::Config &cfg);
#endif
    [[nodiscard]] ResultBool setSecondaryHeader(std::shared_ptr<SecondaryHeaderAbstract> header);

    SecondaryHeaderFactory &getSecondaryHeaderFactory() { return m_secondaryHeaderFactory; }
    [[nodiscard]] const SecondaryHeaderFactory &getSecondaryHeaderFactory() const {
      return m_secondaryHeaderFactory;
    }
    [[nodiscard]] const pus::SecondaryHeaderFactory &getPusSecondaryHeaderFactory() const {
      return m_pusSecondaryHeaderFactory;
    }

    void setDataPacketSize(const std::uint16_t &value);
    void setSecondaryHeaderAutoUpdateStatus(const bool enable) {
      m_enableSecondaryHeaderUpdate = enable;
    }

    [[nodiscard]] std::uint16_t getDataFieldAbsoluteBytesSize() const;
    [[nodiscard]] std::uint16_t getApplicationDataBytesSize() const;
    [[nodiscard]] std::uint16_t getDataFieldUsedBytesSize() const;
    [[nodiscard]] std::uint16_t getDataFieldAvailableBytesSize() const;

    std::vector<std::uint8_t> getSecondaryHeaderBytes();
    [[nodiscard]] std::vector<std::uint8_t> getSecondaryHeaderBytes() const;
    [[nodiscard]] ResultBuffer serialize();
    std::vector<std::uint8_t> getApplicationData();
    [[nodiscard]] std::vector<std::uint8_t> getApplicationData() const;

    [[nodiscard]] bool getSecondaryHeaderAutoUpdateStatus() const {
      return m_enableSecondaryHeaderUpdate;
    }
    [[nodiscard]] bool getSecondaryHeaderFlag() const { return m_secondaryHeader != nullptr; }

    std::shared_ptr<SecondaryHeaderAbstract> getSecondaryHeader();
    [[nodiscard]] std::shared_ptr<const SecondaryHeaderAbstract> getSecondaryHeader() const;

    void update();

  private:
    friend class Packet;
    void clearContent();

    std::shared_ptr<SecondaryHeaderAbstract> m_secondaryHeader{};
    SecondaryHeaderFactory m_secondaryHeaderFactory;
    pus::SecondaryHeaderFactory m_pusSecondaryHeaderFactory;
    std::vector<std::uint8_t> m_applicationData{};
    std::string m_secondaryHeaderType{};
    std::uint16_t m_dataPacketSize{2024};
    bool m_secondaryHeaderUpdated{false};
    bool m_enableSecondaryHeaderUpdate{true};
  };
}

#endif // CCSDS_DATA_FIELD_H
