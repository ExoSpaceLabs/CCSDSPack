// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef PUS_TAILORING_H
#define PUS_TAILORING_H

#include "CCSDSPacketTypes.h"
#include "CCSDSResult.h"
#include "CCSDSTime.h"
#include <cstdint>
#include <string>

namespace ccsds::pus {

  /** @brief Supported standards-facing ECSS PUS revisions. */
  enum class Revision : std::uint8_t {
    Unspecified = 0,
    A = 1,
    C = 2
  };

  /** @brief Returns the canonical selector for one PUS revision and packet direction. */
  [[nodiscard]] inline std::string selector(const Revision revision,
                                            const PacketDirection direction) {
    const char *rev = revision == Revision::A ? "revA"
                      : revision == Revision::C ? "revC" : "unspecified";
    const char *dir = direction == PacketDirection::Telecommand ? "TC"
                      : direction == PacketDirection::Telemetry ? "TM" : "unspecified";
    return std::string{"PUS:"} + rev + ":" + dir;
  }

  namespace rev_a {

    /** @brief Optional ECSS-E-70-41A telecommand layout tailoring. */
    struct TcTailoring {
      std::uint8_t sourceIdOctets{0};
      std::uint8_t secondaryHeaderSpareOctets{0};
    };

    /** @brief Optional ECSS-E-70-41A telemetry layout tailoring. */
    struct TmTailoring {
      std::uint8_t destinationIdOctets{0};
      bool packetSubcounterPresent{false};
      bool timestampPresent{false};
      time::CucConfiguration cuc{};
      std::uint8_t secondaryHeaderSpareOctets{0};
    };

  } // namespace rev_a

  namespace rev_c {

    /** @brief Optional ECSS-E-ST-70-41C telecommand layout tailoring. */
    struct TcTailoring {
      std::uint8_t secondaryHeaderSpareOctets{0};
    };

    /** @brief Optional ECSS-E-ST-70-41C telemetry layout tailoring. */
    struct TmTailoring {
      bool timestampPresent{false};
      time::CucConfiguration cuc{};
      std::uint8_t secondaryHeaderSpareOctets{0};
    };

  } // namespace rev_c

  [[nodiscard]] inline bool validIdentifierWidth(const std::uint8_t width) noexcept {
    return width == 0U || width == 1U || width == 2U || width == 4U;
  }

  [[nodiscard]] inline ResultBool validateTailoring(const rev_a::TcTailoring &tailoring) {
    RET_IF_ERR_MSG(!validIdentifierWidth(tailoring.sourceIdOctets),
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "PUS-A TC source-ID width must be 0, 1, 2, or 4 octets.");
    return true;
  }

  [[nodiscard]] inline ResultBool validateTailoring(const rev_a::TmTailoring &tailoring) {
    RET_IF_ERR_MSG(!validIdentifierWidth(tailoring.destinationIdOctets),
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "PUS-A TM destination-ID width must be 0, 1, 2, or 4 octets.");
    if (tailoring.timestampPresent) {
      FORWARD_RESULT(time::validate(tailoring.cuc));
    } else {
      RET_IF_ERR_MSG(tailoring.cuc.epoch != time::Epoch::Unspecified
                     || tailoring.cuc.pField != time::PFieldMode::Implicit
                     || tailoring.cuc.coarseOctets != 0U
                     || tailoring.cuc.fineOctets != 0U,
                     ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "Disabled PUS-A TM time requires an empty CUC configuration.");
    }
    return true;
  }

  [[nodiscard]] inline ResultBool validateTailoring(const rev_c::TcTailoring &) {
    return true;
  }

  [[nodiscard]] inline ResultBool validateTailoring(const rev_c::TmTailoring &tailoring) {
    if (tailoring.timestampPresent) {
      FORWARD_RESULT(time::validate(tailoring.cuc));
    } else {
      RET_IF_ERR_MSG(tailoring.cuc.epoch != time::Epoch::Unspecified
                     || tailoring.cuc.pField != time::PFieldMode::Implicit
                     || tailoring.cuc.coarseOctets != 0U
                     || tailoring.cuc.fineOctets != 0U,
                     ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                     "Disabled PUS-C TM time requires an empty CUC configuration.");
    }
    return true;
  }

} // namespace ccsds::pus

#endif // PUS_TAILORING_H
