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
    const auto *lhsTc = lhs.asTc();
    const auto *rhsTc = rhs.asTc();
    return lhsTc != nullptr && rhsTc != nullptr
        && lhsTc->getSourceIdOctets() == rhsTc->getSourceIdOctets();
  }

  if (lhs.getDirection() == PacketDirection::Telemetry) {
    const auto *lhsTm = lhs.asTm();
    const auto *rhsTm = rhs.asTm();
    if (!lhsTm || !rhsTm
        || lhsTm->getDestinationIdOctets() != rhsTm->getDestinationIdOctets()
        || lhsTm->timestampPresent() != rhsTm->timestampPresent()
        || !sameCuc(lhsTm->getCucConfiguration(), rhsTm->getCucConfiguration())) {
      return false;
    }

    return lhs.getRevision() != Revision::A
        || lhsTm->packetSubcounterPresent() == rhsTm->packetSubcounterPresent();
  }

  return false;
}
