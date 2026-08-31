// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaders.h"

namespace {
  bool sameCuc(const ccsds::time::CucConfiguration &lhs,
               const ccsds::time::CucConfiguration &rhs) noexcept {
    return lhs.epoch == rhs.epoch
           && lhs.pField == rhs.pField
           && lhs.coarseOctets == rhs.coarseOctets
           && lhs.fineOctets == rhs.fineOctets;
  }
}

bool ccsds::pus::sameTailoring(const SecondaryHeader &lhs,
                               const SecondaryHeader &rhs) noexcept {
  if (lhs.getRevision() != rhs.getRevision()
      || lhs.getDirection() != rhs.getDirection()
      || lhs.getSecondaryHeaderSpareOctets() != rhs.getSecondaryHeaderSpareOctets()) {
    return false;
  }

  if (lhs.getDirection() == PacketDirection::Telecommand) {
    const auto &lhsTc = static_cast<const TcSecondaryHeader &>(lhs);
    const auto &rhsTc = static_cast<const TcSecondaryHeader &>(rhs);
    return lhsTc.getSourceIdOctets() == rhsTc.getSourceIdOctets();
  }

  if (lhs.getDirection() == PacketDirection::Telemetry) {
    const auto &lhsTm = static_cast<const TmSecondaryHeader &>(lhs);
    const auto &rhsTm = static_cast<const TmSecondaryHeader &>(rhs);
    if (lhsTm.getDestinationIdOctets() != rhsTm.getDestinationIdOctets()
        || lhsTm.timestampPresent() != rhsTm.timestampPresent()
        || !sameCuc(lhsTm.getCucConfiguration(), rhsTm.getCucConfiguration())) {
      return false;
    }

    if (lhs.getRevision() == Revision::A) {
      const auto &lhsATm = static_cast<const rev_a::TmHeader &>(lhs);
      const auto &rhsATm = static_cast<const rev_a::TmHeader &>(rhs);
      return lhsATm.packetSubcounterPresent() == rhsATm.packetSubcounterPresent();
    }
    return true;
  }

  return false;
}
