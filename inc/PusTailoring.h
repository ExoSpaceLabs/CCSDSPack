// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef PUS_TAILORING_H
#define PUS_TAILORING_H

#include "CCSDSPacketTypes.h"
#include "CCSDSResult.h"
#include "CCSDSTime.h"
#include "ccsdspack/c/pus_tc.h"
#include "ccsdspack/c/pus_tm.h"
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
    const ccsds_pus_a_tc_tailoring_t core{
      tailoring.sourceIdOctets,
      tailoring.secondaryHeaderSpareOctets
    };
    RET_IF_ERR_MSG(ccsds_pus_a_tc_validate_tailoring(&core) != CCSDS_STATUS_OK,
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "PUS-A TC source-ID width must be 0, 1, 2, or 4 octets.");
    return true;
  }

  [[nodiscard]] inline ResultBool validateTailoring(const rev_a::TmTailoring &tailoring) {
    const ccsds_pus_a_tm_tailoring_t core{
      tailoring.destinationIdOctets,
      static_cast<std::uint8_t>(tailoring.packetSubcounterPresent ? 1U : 0U),
      static_cast<std::uint8_t>(tailoring.timestampPresent ? 1U : 0U),
      {
        static_cast<ccsds_cuc_epoch_t>(tailoring.cuc.epoch),
        static_cast<ccsds_cuc_pfield_mode_t>(tailoring.cuc.pField),
        tailoring.cuc.coarseOctets,
        tailoring.cuc.fineOctets
      },
      tailoring.secondaryHeaderSpareOctets
    };
    const auto status = ccsds_pus_a_tm_validate_tailoring(&core);
    RET_IF_ERR_MSG(status == CCSDS_STATUS_PUS_INVALID_IDENTIFIER_WIDTH,
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "PUS-A TM destination-ID width must be 0, 1, 2, or 4 octets.");
    RET_IF_ERR_MSG(status == CCSDS_STATUS_PUS_DISABLED_TIME_CONFIG,
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Disabled PUS-A TM time requires an empty CUC configuration.");
    if (status != CCSDS_STATUS_OK) {
      FORWARD_RESULT(time::validate(tailoring.cuc));
      return Error{ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Invalid PUS-A TM tailoring."};
    }
    return true;
  }

  [[nodiscard]] inline ResultBool validateTailoring(const rev_c::TcTailoring &) {
    return true;
  }

  [[nodiscard]] inline ResultBool validateTailoring(const rev_c::TmTailoring &tailoring) {
    const ccsds_pus_c_tm_tailoring_t core{
      static_cast<std::uint8_t>(tailoring.timestampPresent ? 1U : 0U),
      {
        static_cast<ccsds_cuc_epoch_t>(tailoring.cuc.epoch),
        static_cast<ccsds_cuc_pfield_mode_t>(tailoring.cuc.pField),
        tailoring.cuc.coarseOctets,
        tailoring.cuc.fineOctets
      },
      tailoring.secondaryHeaderSpareOctets
    };
    const auto status = ccsds_pus_c_tm_validate_tailoring(&core);
    RET_IF_ERR_MSG(status == CCSDS_STATUS_PUS_DISABLED_TIME_CONFIG,
                   ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Disabled PUS-C TM time requires an empty CUC configuration.");
    if (status != CCSDS_STATUS_OK) {
      FORWARD_RESULT(time::validate(tailoring.cuc));
      return Error{ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                   "Invalid PUS-C TM tailoring."};
    }
    return true;
  }

} // namespace ccsds::pus

#endif // PUS_TAILORING_H
