// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file PusSecondaryHeaders.h
 * @brief Defines standards-oriented PUS-A and PUS-C TC/TM secondary headers.
 */
#ifndef PUS_SECONDARY_HEADERS_H
#define PUS_SECONDARY_HEADERS_H

#include "CCSDSSecondaryHeaderAbstract.h"
#include "PusTailoring.h"
#include <cstdint>
#include <string>
#include <vector>

namespace ccsds::pus {

  class SecondaryHeader : public ccsds::SecondaryHeaderAbstract {
  public:
    [[nodiscard]] virtual Revision getRevision() const noexcept = 0;
    [[nodiscard]] std::string getType() const override {
      return selector(getRevision(), getDirection());
    }
    [[nodiscard]] bool isPusHeader() const override { return true; }
    [[nodiscard]] std::uint8_t getSecondaryHeaderSpareOctets() const noexcept {
      return m_secondaryHeaderSpareOctets;
    }
    void update(DataField *dataField) override { (void)dataField; }
#ifndef CCSDS_MCU
    ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif
  protected:
    explicit SecondaryHeader(const std::uint8_t spareOctets = 0U)
      : m_secondaryHeaderSpareOctets(spareOctets) {}
    [[nodiscard]] bool identifierFits(std::uint32_t value, std::uint8_t octets) const;
    static void appendIdentifier(std::vector<std::uint8_t> &bytes,
                                 std::uint32_t value, std::uint8_t octets);
    static std::uint32_t readIdentifier(const std::vector<std::uint8_t> &bytes,
                                        std::size_t offset, std::uint8_t octets);
    [[nodiscard]] bool trailingSpareIsZero(const std::vector<std::uint8_t> &bytes) const;
    std::uint8_t m_secondaryHeaderSpareOctets{0U};
  };

  class TcSecondaryHeader : public SecondaryHeader {
  public:
    static constexpr PacketDirection Direction = PacketDirection::Telecommand;
    [[nodiscard]] PacketDirection getDirection() const noexcept override { return Direction; }
    [[nodiscard]] std::uint8_t getAcknowledgementFlags() const { return m_acknowledgementFlags; }
    [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
    [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubtype; }
    [[nodiscard]] std::uint32_t getSourceId() const { return m_sourceId; }
    [[nodiscard]] std::uint8_t getSourceIdOctets() const noexcept { return m_sourceIdOctets; }
    ResultBool setAcknowledgementFlags(std::uint8_t flags);
    void setServiceType(std::uint8_t value) { m_serviceType = value; }
    void setServiceSubtype(std::uint8_t value) { m_serviceSubtype = value; }
    ResultBool setSourceId(std::uint32_t value);
#ifndef CCSDS_MCU
    ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif
  protected:
    TcSecondaryHeader(std::uint8_t sourceIdOctets, std::uint8_t spareOctets,
                      std::uint8_t serviceType, std::uint8_t serviceSubtype,
                      std::uint32_t sourceId, std::uint8_t acknowledgementFlags);
    [[nodiscard]] std::uint16_t tcSize() const;
    void appendTcBody(std::vector<std::uint8_t> &bytes) const;
    ResultBool parseTcBody(const std::vector<std::uint8_t> &data, std::size_t offset);
    std::uint8_t m_sourceIdOctets{0U};
    std::uint8_t m_acknowledgementFlags{0U};
    std::uint8_t m_serviceType{0U};
    std::uint8_t m_serviceSubtype{0U};
    std::uint32_t m_sourceId{0U};
  };

  class TmSecondaryHeader : public SecondaryHeader {
  public:
    static constexpr PacketDirection Direction = PacketDirection::Telemetry;
    [[nodiscard]] PacketDirection getDirection() const noexcept override { return Direction; }
    [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
    [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubtype; }
    [[nodiscard]] std::uint32_t getDestinationId() const { return m_destinationId; }
    [[nodiscard]] std::uint8_t getDestinationIdOctets() const noexcept { return m_destinationIdOctets; }
    [[nodiscard]] bool timestampPresent() const noexcept { return m_timestampPresent; }
    [[nodiscard]] const time::CucConfiguration &getCucConfiguration() const noexcept { return m_cuc; }
    [[nodiscard]] const time::CucTime &getTimestamp() const { return m_timestamp; }
    void setServiceType(std::uint8_t value) { m_serviceType = value; }
    void setServiceSubtype(std::uint8_t value) { m_serviceSubtype = value; }
    ResultBool setDestinationId(std::uint32_t value);
    ResultBool setTimestamp(const time::CucTime &timestamp);
#ifndef CCSDS_MCU
    ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif
  protected:
    TmSecondaryHeader(std::uint8_t destinationIdOctets, bool timestampPresent,
                      time::CucConfiguration cuc, std::uint8_t spareOctets,
                      std::uint8_t serviceType, std::uint8_t serviceSubtype,
                      std::uint32_t destinationId, time::CucTime timestamp);
    ResultBool appendTmTail(std::vector<std::uint8_t> &bytes) const;
    ResultBool parseTmTail(const std::vector<std::uint8_t> &data, std::size_t offset);
    [[nodiscard]] std::uint16_t tmTailSize() const;
    std::uint8_t m_destinationIdOctets{0U};
    bool m_timestampPresent{false};
    time::CucConfiguration m_cuc{};
    std::uint8_t m_serviceType{0U};
    std::uint8_t m_serviceSubtype{0U};
    std::uint32_t m_destinationId{0U};
    time::CucTime m_timestamp{};
  };

  namespace rev_a {
    class TcHeader final : public TcSecondaryHeader {
    public:
      static constexpr Revision RevisionValue = Revision::A;
      TcHeader();
      TcHeader(std::uint8_t serviceType, std::uint8_t serviceSubtype,
               std::uint32_t sourceId = 0U, std::uint8_t acknowledgementFlags = 0U);
      explicit TcHeader(TcTailoring tailoring, std::uint8_t serviceType = 0U,
                        std::uint8_t serviceSubtype = 0U, std::uint32_t sourceId = 0U,
                        std::uint8_t acknowledgementFlags = 0U);
      [[nodiscard]] Revision getRevision() const noexcept override { return RevisionValue; }
      [[nodiscard]] TcTailoring getTailoring() const noexcept {
        return {m_sourceIdOctets, m_secondaryHeaderSpareOctets};
      }
      [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
      [[nodiscard]] std::uint16_t getSize() const override { return tcSize(); }
      [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
    };

    class TmHeader final : public TmSecondaryHeader {
    public:
      static constexpr Revision RevisionValue = Revision::A;
      TmHeader();
      TmHeader(std::uint8_t serviceType, std::uint8_t serviceSubtype,
               std::uint8_t packetSubcounter = 0U, std::uint32_t destinationId = 0U,
               time::CucTime timestamp = {});
      explicit TmHeader(TmTailoring tailoring, std::uint8_t serviceType = 0U,
                        std::uint8_t serviceSubtype = 0U, std::uint8_t packetSubcounter = 0U,
                        std::uint32_t destinationId = 0U, time::CucTime timestamp = {});
      [[nodiscard]] Revision getRevision() const noexcept override { return RevisionValue; }
      [[nodiscard]] std::uint8_t getPacketSubcounter() const { return m_packetSubcounter; }
      [[nodiscard]] bool packetSubcounterPresent() const noexcept { return m_packetSubcounterPresent; }
      void setPacketSubcounter(std::uint8_t value) { m_packetSubcounter = value; }
      [[nodiscard]] TmTailoring getTailoring() const noexcept {
        return {m_destinationIdOctets, m_packetSubcounterPresent, m_timestampPresent,
                m_cuc, m_secondaryHeaderSpareOctets};
      }
      [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
      [[nodiscard]] std::uint16_t getSize() const override;
      [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
#ifndef CCSDS_MCU
      ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif
    private:
      bool m_packetSubcounterPresent{false};
      std::uint8_t m_packetSubcounter{0U};
    };
  } // namespace rev_a

  namespace rev_c {
    class TcHeader final : public TcSecondaryHeader {
    public:
      static constexpr Revision RevisionValue = Revision::C;
      TcHeader();
      TcHeader(std::uint8_t serviceType, std::uint8_t serviceSubtype,
               std::uint32_t sourceId = 0U, std::uint8_t acknowledgementFlags = 0U);
      explicit TcHeader(TcTailoring tailoring, std::uint8_t serviceType = 0U,
                        std::uint8_t serviceSubtype = 0U, std::uint32_t sourceId = 0U,
                        std::uint8_t acknowledgementFlags = 0U);
      [[nodiscard]] Revision getRevision() const noexcept override { return RevisionValue; }
      [[nodiscard]] TcTailoring getTailoring() const noexcept { return {m_secondaryHeaderSpareOctets}; }
      [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
      [[nodiscard]] std::uint16_t getSize() const override { return tcSize(); }
      [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
    };

    class TmHeader final : public TmSecondaryHeader {
    public:
      static constexpr Revision RevisionValue = Revision::C;
      TmHeader();
      TmHeader(std::uint8_t serviceType, std::uint8_t serviceSubtype,
               std::uint16_t messageTypeCounter = 0U, std::uint32_t destinationId = 0U,
               std::uint8_t timeReferenceStatus = 0U, time::CucTime timestamp = {});
      explicit TmHeader(TmTailoring tailoring, std::uint8_t serviceType = 0U,
                        std::uint8_t serviceSubtype = 0U,
                        std::uint16_t messageTypeCounter = 0U,
                        std::uint32_t destinationId = 0U,
                        std::uint8_t timeReferenceStatus = 0U,
                        time::CucTime timestamp = {});
      [[nodiscard]] Revision getRevision() const noexcept override { return RevisionValue; }
      [[nodiscard]] std::uint16_t getMessageTypeCounter() const { return m_messageTypeCounter; }
      [[nodiscard]] std::uint8_t getTimeReferenceStatus() const { return m_timeReferenceStatus; }
      void setMessageTypeCounter(std::uint16_t value) { m_messageTypeCounter = value; }
      ResultBool setTimeReferenceStatus(std::uint8_t value);
      [[nodiscard]] TmTailoring getTailoring() const noexcept {
        return {m_timestampPresent, m_cuc, m_secondaryHeaderSpareOctets};
      }
      [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
      [[nodiscard]] std::uint16_t getSize() const override;
      [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
#ifndef CCSDS_MCU
      ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif
    private:
      std::uint16_t m_messageTypeCounter{0U};
      std::uint8_t m_timeReferenceStatus{0U};
    };
  } // namespace rev_c

  /** @brief Returns true when two PUS headers use the same identity and wire-layout tailoring. */
  [[nodiscard]] bool sameTailoring(const SecondaryHeader &lhs,
                                   const SecondaryHeader &rhs) noexcept;

} // namespace ccsds::pus

#endif // PUS_SECONDARY_HEADERS_H
