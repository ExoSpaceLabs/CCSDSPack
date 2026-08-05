// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file PusSecondaryHeaders.h
 * @brief Defines standards-oriented PUS-A and PUS-C TC/TM secondary headers.
 */
#ifndef PUS_SECONDARY_HEADERS_H
#define PUS_SECONDARY_HEADERS_H

#include "CCSDSMissionProfile.h"
#include "CCSDSSecondaryHeaderAbstract.h"
#include <cstdint>
#include <string>
#include <vector>

namespace ccsds::pus {

  /** @brief Common profile-aware base for standards-defined PUS headers. */
  class SecondaryHeader : public ccsds::SecondaryHeaderAbstract {
  public:
    /** @brief Returns the ECSS PUS revision selected by the mission profile. */
    [[nodiscard]] Revision getRevision() const { return m_profile.pusRevision; }
    /** @brief Returns the packet direction selected by the mission profile. */
    [[nodiscard]] Direction getDirection() const { return m_profile.direction; }
    /** @brief Returns the immutable mission profile used by this header. */
    [[nodiscard]] const MissionProfile &getMissionProfile() const { return m_profile; }
    /** @brief Returns the canonical PUS revision/direction selector. */
    [[nodiscard]] std::string getType() const override {
      return selector(m_profile.pusRevision, m_profile.direction);
    }
    /** @brief Identifies this instance as a standards-defined PUS header. */
    [[nodiscard]] bool isPusHeader() const override { return true; }
    /** @brief Returns whether the supplied profile matches this header exactly. */
    [[nodiscard]] bool matchesMissionProfile(const MissionProfile &profile) const override {
      return missionProfilesEqual(m_profile, profile);
    }
    /** @brief Compatibility alias for matchesMissionProfile(). */
    [[nodiscard]] bool matchesProfile(const MissionProfile &profile) const {
      return matchesMissionProfile(profile);
    }
    /** @brief Performs no application-data-derived update for fixed PUS fields. */
    void update(DataField *dataField) override { (void)dataField; }

#ifndef CCSDS_MCU
    /** @brief Validates that the configuration selects this header profile. */
    ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif

  protected:
    explicit SecondaryHeader(MissionProfile profile) : m_profile(profile) {}
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

  /** @brief Shared service and source fields for PUS telecommand headers. */
  class TcSecondaryHeader : public SecondaryHeader {
  public:
    /** @brief Returns the four PUS acknowledgement flags. */
    [[nodiscard]] std::uint8_t getAcknowledgementFlags() const { return m_acknowledgementFlags; }
    /** @brief Returns the PUS service type. */
    [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
    /** @brief Returns the PUS service subtype. */
    [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubtype; }
    /** @brief Returns the mission-tailored source identifier. */
    [[nodiscard]] std::uint32_t getSourceId() const { return m_sourceId; }

    /** @brief Sets the four PUS acknowledgement flags. */
    ResultBool setAcknowledgementFlags(std::uint8_t flags);
    /** @brief Sets the PUS service type. */
    void setServiceType(std::uint8_t value) { m_serviceType = value; }
    /** @brief Sets the PUS service subtype. */
    void setServiceSubtype(std::uint8_t value) { m_serviceSubtype = value; }
    /** @brief Sets the source identifier when it fits the mission profile. */
    ResultBool setSourceId(std::uint32_t value);

#ifndef CCSDS_MCU
    /** @brief Loads common PUS telecommand fields. */
    ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif

  protected:
    TcSecondaryHeader(MissionProfile profile, std::uint8_t serviceType,
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

  /** @brief Shared service, destination, and time fields for PUS telemetry headers. */
  class TmSecondaryHeader : public SecondaryHeader {
  public:
    /** @brief Returns the PUS service type. */
    [[nodiscard]] std::uint8_t getServiceType() const { return m_serviceType; }
    /** @brief Returns the PUS service subtype. */
    [[nodiscard]] std::uint8_t getServiceSubtype() const { return m_serviceSubtype; }
    /** @brief Returns the mission-tailored destination identifier. */
    [[nodiscard]] std::uint32_t getDestinationId() const { return m_destinationId; }
    /** @brief Returns the numeric CUC telemetry timestamp. */
    [[nodiscard]] const time::CucTime &getTimestamp() const { return m_timestamp; }

    /** @brief Sets the PUS service type. */
    void setServiceType(std::uint8_t value) { m_serviceType = value; }
    /** @brief Sets the PUS service subtype. */
    void setServiceSubtype(std::uint8_t value) { m_serviceSubtype = value; }
    /** @brief Sets the destination identifier when it fits the mission profile. */
    ResultBool setDestinationId(std::uint32_t value);
    /** @brief Sets a numeric timestamp when it fits the mission CUC layout. */
    ResultBool setTimestamp(const time::CucTime &timestamp);

#ifndef CCSDS_MCU
    /** @brief Loads common PUS telemetry fields and the numeric CUC value. */
    ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif

  protected:
    TmSecondaryHeader(MissionProfile profile, std::uint8_t serviceType,
                      std::uint8_t serviceSubtype, std::uint32_t destinationId,
                      time::CucTime timestamp);
    ResultBool appendTmTail(std::vector<std::uint8_t> &bytes) const;
    ResultBool parseTmTail(const std::vector<std::uint8_t> &data, std::size_t offset);
    [[nodiscard]] std::uint16_t tmTailSize() const;

    std::uint8_t m_serviceType{0};
    std::uint8_t m_serviceSubtype{0};
    std::uint32_t m_destinationId{0};
    time::CucTime m_timestamp{};
  };

  namespace rev_a {
  /** @brief ECSS-E-70-41A telecommand secondary header. */
  class TcHeader final : public TcSecondaryHeader {
  public:
    /** @brief Constructs a PUS-A telecommand secondary header. */
    explicit TcHeader(MissionProfile profile = makeProfile(
                        Revision::A, Direction::Telecommand),
                      std::uint8_t serviceType = 0, std::uint8_t serviceSubtype = 0,
                      std::uint32_t sourceId = 0, std::uint8_t acknowledgementFlags = 0);
    /** @brief Parses a complete PUS-A telecommand secondary header. */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
    /** @brief Returns the profile-derived encoded header size. */
    [[nodiscard]] std::uint16_t getSize() const override { return tcSize(); }
    /** @brief Serializes the PUS-A telecommand secondary header. */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

  };
  } // namespace rev_a

  namespace rev_c {
  /** @brief ECSS-E-ST-70-41C telecommand secondary header. */
  class TcHeader final : public TcSecondaryHeader {
  public:
    /** @brief Constructs a PUS-C telecommand secondary header. */
    explicit TcHeader(MissionProfile profile = makeProfile(
                        Revision::C, Direction::Telecommand),
                      std::uint8_t serviceType = 0, std::uint8_t serviceSubtype = 0,
                      std::uint32_t sourceId = 0, std::uint8_t acknowledgementFlags = 0);
    /** @brief Parses a complete PUS-C telecommand secondary header. */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
    /** @brief Returns the profile-derived encoded header size. */
    [[nodiscard]] std::uint16_t getSize() const override { return tcSize(); }
    /** @brief Serializes the PUS-C telecommand secondary header. */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

  };
  } // namespace rev_c

  namespace rev_a {
  /** @brief ECSS-E-70-41A telemetry secondary header. */
  class TmHeader final : public TmSecondaryHeader {
  public:
    /** @brief Constructs a PUS-A telemetry secondary header. */
    explicit TmHeader(MissionProfile profile = makeProfile(
                        Revision::A, Direction::Telemetry),
                      std::uint8_t serviceType = 0, std::uint8_t serviceSubtype = 0,
                      std::uint8_t packetSubcounter = 0, std::uint32_t destinationId = 0,
                      time::CucTime timestamp = {});
    /** @brief Returns the optional PUS-A packet subcounter. */
    [[nodiscard]] std::uint8_t getPacketSubcounter() const { return m_packetSubcounter; }
    /** @brief Sets the optional PUS-A packet subcounter. */
    void setPacketSubcounter(std::uint8_t value) { m_packetSubcounter = value; }
    /** @brief Parses a complete PUS-A telemetry secondary header. */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
    /** @brief Returns the profile-derived encoded header size. */
    [[nodiscard]] std::uint16_t getSize() const override;
    /** @brief Serializes the PUS-A telemetry secondary header. */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

#ifndef CCSDS_MCU
    /** @brief Loads the optional PUS-A packet subcounter. */
    ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif

  private:
    std::uint8_t m_packetSubcounter{0};
  };
  } // namespace rev_a

  namespace rev_c {
  /** @brief ECSS-E-ST-70-41C telemetry secondary header. */
  class TmHeader final : public TmSecondaryHeader {
  public:
    /** @brief Constructs a PUS-C telemetry secondary header. */
    explicit TmHeader(MissionProfile profile = makeProfile(
                        Revision::C, Direction::Telemetry),
                      std::uint8_t serviceType = 0, std::uint8_t serviceSubtype = 0,
                      std::uint16_t messageTypeCounter = 0,
                      std::uint32_t destinationId = 0,
                      std::uint8_t timeReferenceStatus = 0,
                      time::CucTime timestamp = {});
    /** @brief Returns the PUS-C message-type counter. */
    [[nodiscard]] std::uint16_t getMessageTypeCounter() const { return m_messageTypeCounter; }
    /** @brief Returns the PUS-C time-reference status. */
    [[nodiscard]] std::uint8_t getTimeReferenceStatus() const { return m_timeReferenceStatus; }
    /** @brief Sets the PUS-C message-type counter. */
    void setMessageTypeCounter(std::uint16_t value) { m_messageTypeCounter = value; }
    /** @brief Sets the three-bit PUS-C time-reference status. */
    ResultBool setTimeReferenceStatus(std::uint8_t value);
    /** @brief Parses a complete PUS-C telemetry secondary header. */
    [[nodiscard]] ResultBool deserialize(const std::vector<std::uint8_t> &data) override;
    /** @brief Returns the profile-derived encoded header size. */
    [[nodiscard]] std::uint16_t getSize() const override;
    /** @brief Serializes the PUS-C telemetry secondary header. */
    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;

#ifndef CCSDS_MCU
    /** @brief Loads the PUS-C message counter and time-reference status. */
    ResultBool loadFromConfig(const ccsds::Config &config) override;
#endif

  private:
    std::uint16_t m_messageTypeCounter{0};
    std::uint8_t m_timeReferenceStatus{0};
  };
  } // namespace rev_c

} // namespace ccsds::pus

#endif // PUS_SECONDARY_HEADERS_H
