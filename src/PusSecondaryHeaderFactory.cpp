// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaderFactory.h"

ccsds::Result<std::shared_ptr<ccsds::SecondaryHeaderAbstract>>
ccsds::pus::SecondaryHeaderFactory::create(const Revision revision,
                                           const Direction direction,
                                           const MissionProfile &profile) const {
  const auto profileResult = validateMissionProfile(profile);
  if (!profileResult) return profileResult.error();
  RET_IF_ERR_MSG(profile.pusRevision != revision || profile.direction != direction,
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS factory selection does not match the mission profile.");

  if (revision == Revision::A && direction == Direction::Telecommand)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<rev_a::TcHeader>(profile));
  if (revision == Revision::A && direction == Direction::Telemetry)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<rev_a::TmHeader>(profile));
  if (revision == Revision::C && direction == Direction::Telecommand)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<rev_c::TcHeader>(profile));
  if (revision == Revision::C && direction == Direction::Telemetry)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<rev_c::TmHeader>(profile));
  return Error{ErrorCode::INVALID_SECONDARY_HEADER_DATA, "Unsupported PUS revision/direction selection."};
}

ccsds::Result<std::shared_ptr<ccsds::SecondaryHeaderAbstract>>
ccsds::pus::SecondaryHeaderFactory::create(const std::string &selector,
                                         const MissionProfile &profile) const {
  RET_IF_ERR_MSG(!typeIsSupported(selector), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Unsupported PUS secondary-header selector: " + selector);
  RET_IF_ERR_MSG(selector != ccsds::pus::selector(profile.pusRevision, profile.direction),
                 ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "PUS selector does not match the mission profile.");
  return create(profile.pusRevision, profile.direction, profile);
}

bool ccsds::pus::SecondaryHeaderFactory::typeIsSupported(const std::string &selector) const {
  return selector == "PUS:revA:TC" || selector == "PUS:revA:TM"
         || selector == "PUS:revC:TC" || selector == "PUS:revC:TM";
}
