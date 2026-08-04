// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef PUS_SECONDARY_HEADERS_H
#define PUS_SECONDARY_HEADERS_H

#include "CCSDSMissionProfile.h"
#include "CCSDSSecondaryHeaderAbstract.h"
#include <cstdint>
#include <string>
#include <vector>

namespace CCSDS {

  class PusSecondaryHeader : public SecondaryHeaderAbstract {
  public:
    [[nodiscard]] PusRevision getRevision() const { return m_profile.pusRevision; }
    [[nodiscard]] PacketDirection getDirection() const { return m_profile.direction; }
    [[nodiscard]] const MissionProfile &getMissionProfile() const { return m_profile; }
    [[nodiscard]] std::string getType() const override {
      return pusSelector(m_profile.pusRevision, m_profile.direction);
    }
    [[nodiscard]] bool isPusHeader() const override { return true; }
    [[nodiscard]] bool matchesMissionProfile(const MissionProfile &profile) const override {
      return missionProfilesEqual(m_profile, profile);
    }
    [[nodiscard]] bool matchesProfile(const MissionProfile &profile) const {
      return matchesMissionProfile(profile);
    }
    void update(DataField *dataField) override { (void)dataField; }

#ifndef CCSDS_MCU
    ResultBool loadFromConfig(const Config &config) override;
#endif

  protected:
    explicit PusSecondaryHeader(MissionProfile profile) : m_profile(profile) {}
    [[nodiscard]] bool profileIsValid() const {
      return static_cast<bool>(validateMissionProfile(m_profile));
    }
    [[nodiscard]] bool identifierFits(std::uint32_t value, std::uint8_t octets) const;
    static void appendIdentifier(std::vector<std::uint8_t> &bytes,
                                 std::uint32_t value, std::uint8_t octets);
    static std::uint32_t readIdentifier(const std::vector<std::uint8_t> &bytes,
                                        std::size_t offset, std::uint8_t octets);
    [[nodiscard]] bool trailingSpareIsZero(const std::vector<std::uint8_t> &bytes) const;

    MissionProfile m_profile;
  };

  class PusTcSecondaryHeader : public PusSecondaryHeader {
  public:
    [[nodiscard]] std::uint8_t getAcknowledgementFlags() const { return m_acknowledgementFlags; }
    [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
    [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubtype; }
    [[nodiscard]] std::uint32_t getSourceId() const { return m_sourceId; }

    ResultBool setAcknowledgementFlags(std::uint8_t flags);
    void setServiceType(std::uint8_t value) { m_serviceType = value; }
    void setServiceSubtype(std::uint8_t value) { m_serviceSubtype = value; }
    ResultBool setSourceId(std::uint32_t value);

  protected:
    PusTcSecondaryHeader(MissionProfile profile, std::uint8_t serviceType,
                         std::uint8_t serviceSubtype, std::uint32_t sourceId,
                         std::uint8_t acknowledgementFlags);
    [[nodiscard]] std::uint16_t tcSize() const;
    void appendTcBody(std::vector<std::uint8_t> &bytes) const;
    ResultBool parseTcBody(const std::vector<std::uint8_t> &data, std::size_t offset);

    std::uint8_t m_acknowledgementFlags{0};
    std::uint8_t m_serviceType{0};
    std::uint8_t m_serviceSubtype{0};
    std::uint32_t m_sourceId{0};
  };

  class PusTmSecondaryHeader : public PusSecondaryHeader {
  public:
    [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
    [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubtype; }
    [[nodiscard]] std::uint32_t getDestinationId() const { return m_destinationId; }
    [[nodiscard]] const std::vector<std::uint8_t> &getTimestamp() const { return m_timestamp; }

    void setServiceType(std::uint8_t value) { m_serviceType = value; }
    void setServiceSubtype(std::uint8_t value) { m_serviceSubtype = value; }
    ResultBool setDestinationId(std::uint32_t value);
    ResultBool setTimestamp(const std::vector<std::uint8_t> &timestamp);

  protected:
    PusTmSecondaryHeader(MissionProfile profile, std::uint8_t serviceType,
                         std::uint8_t serviceSubtype, std::uint32_t destinationId,
                         std::vector<std::uint8_t> timestamp);
    void appendTmTail(std::vector<std::uint8_t> &bytes) const;
    ResultBool parseTmTail(const std::vector<std::uint8_t> &data, std::size_t offset);
    [[nodiscard]] std::uint16_t tmTailSize() const;

    std::uint8_t m_serviceType{0};
    std::uint8_t m_serviceSubtype{0};
    std::uint32_t m_destinationId{0};
    std::vector<std::uint8_t> m_timestamp{};
  };

  class PusATcHeader final : public PusTcSecondaryHeader {
  public:
    explicit PusATcHeader(MissionProfile profile = makePusProfile(
                            PusRevision::A, PacketDirection::Telecommand),
                          std::uint8_t serviceType = 0, std::uint8_t serviceSubtype = 0,
                          std::uint32_t sourceId = 0, std::uint8_t acknowledgementFlags = 0);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
    [[nodiscard]] std::uint16_t getSize() const override { return tcSize(); }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
  };

  class PusCTcHeader final : public PusTcSecondaryHeader {
  public:
    explicit PusCTcHeader(MissionProfile profile = makePusProfile(
                            PusRevision::C, PacketDirection::Telecommand),
                          std::uint8_t serviceType = 0, std::uint8_t serviceSubtype = 0,
                          std::uint32_t sourceId = 0, std::uint8_t acknowledgementFlags = 0);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
    [[nodiscard]] std::uint16_t getSize() const override { return tcSize(); }
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
  };

  class PusATmHeader final : public PusTmSecondaryHeader {
  public:
    explicit PusATmHeader(MissionProfile profile = makePusProfile(
                            PusRevision::A, PacketDirection::Telemetry),
                          std::uint8_t serviceType = 0, std::uint8_t serviceSubtype = 0,
                          std::uint8_t packetSubcounter = 0, std::uint32_t destinationId = 0,
                          std::vector<std::uint8_t> timestamp = {});
    [[nodiscard]] std::uint8_t getPacketSubcounter() const { return m_packetSubcounter; }
    void setPacketSubcounter(std::uint8_t value) { m_packetSubcounter = value; }
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
    [[nodiscard]] std::uint16_t getSize() const override;
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

  private:
    std::uint8_t m_packetSubcounter{0};
  };

  class PusCTmHeader final : public PusTmSecondaryHeader {
  public:
    explicit PusCTmHeader(MissionProfile profile = makePusProfile(
                            PusRevision::C, PacketDirection::Telemetry),
                          std::uint8_t serviceType = 0, std::uint8_t serviceSubtype = 0,
                          std::uint16_t messageTypeCounter = 0,
                          std::uint32_t destinationId = 0,
                          std::uint8_t timeReferenceStatus = 0,
                          std::vector<std::uint8_t> timestamp = {});
    [[nodiscard]] std::uint16_t getMessageTypeCounter() const { return m_messageTypeCounter; }
    [[nodiscard]] std::uint8_t getTimeReferenceStatus() const { return m_timeReferenceStatus; }
    void setMessageTypeCounter(std::uint16_t value) { m_messageTypeCounter = value; }
    ResultBool setTimeReferenceStatus(std::uint8_t value);
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
    [[nodiscard]] std::uint16_t getSize() const override;
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

  private:
    std::uint16_t m_messageTypeCounter{0};
    std::uint8_t m_timeReferenceStatus{0};
  };

} // namespace CCSDS

#endif // PUS_SECONDARY_HEADERS_H
