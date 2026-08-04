// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaderFactory.h"

CCSDS::Result<std::shared_ptr<CCSDS::SecondaryHeaderAbstract>>
CCSDS::PusSecondaryHeaderFactory::create(const PusRevision revision,
                                         const PacketDirection direction,
                                         const MissionProfile &profile) const {
  const auto profileResult = validateMissionProfile(profile);
  if (!profileResult) return profileResult.error();
  RET_IF_ERR_MSG(profile.pusRevision != revision || profile.direction != direction,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS factory selection does not match the mission profile.");

  if (revision == PusRevision::A && direction == PacketDirection::Telecommand)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<PusATcHeader>(profile));
  if (revision == PusRevision::A && direction == PacketDirection::Telemetry)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<PusATmHeader>(profile));
  if (revision == PusRevision::C && direction == PacketDirection::Telecommand)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<PusCTcHeader>(profile));
  if (revision == PusRevision::C && direction == PacketDirection::Telemetry)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<PusCTmHeader>(profile));
  return Error{ErrorCode::INVALID_SECONDARY_HEADER_DATA, "Unsupported PUS revision/direction selection."};
}

CCSDS::Result<std::shared_ptr<CCSDS::SecondaryHeaderAbstract>>
CCSDS::PusSecondaryHeaderFactory::create(const std::string &selector,
                                         const MissionProfile &profile) const {
  RET_IF_ERR_MSG(!typeIsSupported(selector), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Unsupported PUS secondary-header selector: " + selector);
  RET_IF_ERR_MSG(selector != pusSelector(profile.pusRevision, profile.direction),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS selector does not match the mission profile.");
  return create(profile.pusRevision, profile.direction, profile);
}

bool CCSDS::PusSecondaryHeaderFactory::typeIsSupported(const std::string &selector) const {
  return selector == "PUS:revA:TC" || selector == "PUS:revA:TM"
         || selector == "PUS:revC:TC" || selector == "PUS:revC:TM";
}
