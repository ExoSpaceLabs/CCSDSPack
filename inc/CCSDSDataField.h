// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

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
  class DataField {
  public:
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

    ~DataField() = default;

    template <typename T>
    ResultBool RegisterSecondaryHeader() {
      FORWARD_RESULT(m_secondaryHeaderFactory.registerType(std::make_shared<T>()));
      return true;
    }

    [[nodiscard]] ResultBool setApplicationData(const std::vector<std::uint8_t> &applicationData);
    [[nodiscard]] ResultBool setApplicationData(const std::uint8_t *pData,
                                                const std::size_t &sizeData);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                const std::size_t &sizeData);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::uint8_t *pData,
                                                const std::size_t &sizeData,
                                                const std::string &pType);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &data,
                                                const std::string &pType);
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<std::uint8_t> &dataFieldHeader);
#ifndef CCSDS_MCU
    [[nodiscard]] ResultBool setDataFieldHeader(const Config &cfg);
#endif
    void setDataFieldHeader(std::shared_ptr<SecondaryHeaderAbstract> header);

    SecondaryHeaderFactory &getDataFieldHeaderFactory() { return m_secondaryHeaderFactory; }
    [[nodiscard]] const SecondaryHeaderFactory &getDataFieldHeaderFactory() const {
      return m_secondaryHeaderFactory;
    }
    SecondaryHeaderAbstract &getDataFieldHeader() { return *m_secondaryHeader; }
    [[nodiscard]] const SecondaryHeaderAbstract &getDataFieldHeader() const { return *m_secondaryHeader; }
    void setDataPacketSize(const std::uint16_t &value);
    void setDataFieldHeaderAutoUpdateStatus(const bool enable) { m_enableDataFieldUpdate = enable; }

    [[nodiscard]] std::uint16_t getDataFieldAbsoluteBytesSize() const;
    [[nodiscard]] std::uint16_t getApplicationDataBytesSize() const;
    [[nodiscard]] std::uint16_t getDataFieldUsedBytesSize() const;
    [[nodiscard]] std::uint16_t getDataFieldAvailableBytesSize() const;
    std::vector<std::uint8_t> getDataFieldHeaderBytes();
    [[nodiscard]] std::vector<std::uint8_t> getDataFieldHeaderBytes() const;
    std::vector<std::uint8_t> serialize();
    std::vector<std::uint8_t> getApplicationData();
    [[nodiscard]] std::vector<std::uint8_t> getApplicationData() const;
    [[nodiscard]] bool getDataFieldHeaderAutoUpdateStatus() const { return m_enableDataFieldUpdate; }
    [[nodiscard]] bool getDataFieldHeaderFlag() const { return m_secondaryHeader != nullptr; }
    std::shared_ptr<SecondaryHeaderAbstract> getSecondaryHeader();
    [[nodiscard]] std::shared_ptr<const SecondaryHeaderAbstract> getSecondaryHeader() const;
    void update();

  private:
    friend class Packet;
    void clearContent();

    std::shared_ptr<SecondaryHeaderAbstract> m_secondaryHeader{};
    SecondaryHeaderFactory m_secondaryHeaderFactory;
    std::vector<std::uint8_t> m_applicationData{};
    std::string m_dataFieldHeaderType{};
    std::uint16_t m_dataPacketSize{2024};
    bool m_dataFieldHeaderUpdated{false};
    bool m_enableDataFieldUpdate{true};
  };
}

#endif // CCSDS_DATA_FIELD_H
