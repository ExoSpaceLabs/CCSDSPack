// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "PusSecondaryHeaderFactory.h"

ccsds::Result<std::shared_ptr<ccsds::SecondaryHeaderAbstract>>
ccsds::pus::SecondaryHeaderFactory::create(const Revision revision,
                                           const PacketDirection direction) const {
  if (revision == Revision::A && direction == PacketDirection::Telecommand)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<rev_a::TcHeader>());
  if (revision == Revision::A && direction == PacketDirection::Telemetry)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<rev_a::TmHeader>());
  if (revision == Revision::C && direction == PacketDirection::Telecommand)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<rev_c::TcHeader>());
  if (revision == Revision::C && direction == PacketDirection::Telemetry)
    return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<rev_c::TmHeader>());
  return Error{ErrorCode::INVALID_SECONDARY_HEADER_DATA,
               "Unsupported PUS revision/direction selection."};
}

ccsds::Result<std::shared_ptr<ccsds::SecondaryHeaderAbstract>>
ccsds::pus::SecondaryHeaderFactory::create(const std::string &value) const {
  RET_IF_ERR_MSG(!typeIsSupported(value), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
                 "Unsupported PUS secondary-header selector: " + value);
  if (value == "PUS:revA:TC") return create(Revision::A, PacketDirection::Telecommand);
  if (value == "PUS:revA:TM") return create(Revision::A, PacketDirection::Telemetry);
  if (value == "PUS:revC:TC") return create(Revision::C, PacketDirection::Telecommand);
  return create(Revision::C, PacketDirection::Telemetry);
}

ccsds::Result<std::shared_ptr<ccsds::SecondaryHeaderAbstract>>
ccsds::pus::SecondaryHeaderFactory::clone(const SecondaryHeader &header) const {
  if (header.getRevision() == Revision::A
      && header.getDirection() == PacketDirection::Telecommand) {
    const auto &typed = static_cast<const rev_a::TcHeader &>(header);
    return std::static_pointer_cast<SecondaryHeaderAbstract>(
      std::make_shared<rev_a::TcHeader>(typed));
  }
  if (header.getRevision() == Revision::A
      && header.getDirection() == PacketDirection::Telemetry) {
    const auto &typed = static_cast<const rev_a::TmHeader &>(header);
    return std::static_pointer_cast<SecondaryHeaderAbstract>(
      std::make_shared<rev_a::TmHeader>(typed));
  }
  if (header.getRevision() == Revision::C
      && header.getDirection() == PacketDirection::Telecommand) {
    const auto &typed = static_cast<const rev_c::TcHeader &>(header);
    return std::static_pointer_cast<SecondaryHeaderAbstract>(
      std::make_shared<rev_c::TcHeader>(typed));
  }
  if (header.getRevision() == Revision::C
      && header.getDirection() == PacketDirection::Telemetry) {
    const auto &typed = static_cast<const rev_c::TmHeader &>(header);
    return std::static_pointer_cast<SecondaryHeaderAbstract>(
      std::make_shared<rev_c::TmHeader>(typed));
  }
  return Error{ErrorCode::INVALID_SECONDARY_HEADER_DATA,
               "Cannot clone unsupported PUS header identity."};
}

bool ccsds::pus::SecondaryHeaderFactory::typeIsSupported(const std::string &value) const {
  return value == "PUS:revA:TC" || value == "PUS:revA:TM"
         || value == "PUS:revC:TC" || value == "PUS:revC:TM";
}
