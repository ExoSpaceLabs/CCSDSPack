// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef PUS_SECONDARY_HEADER_FACTORY_H
#define PUS_SECONDARY_HEADER_FACTORY_H

#include "PusSecondaryHeaders.h"
#include <memory>
#include <string>

namespace CCSDS {

  /** Fixed, non-extensible registry for standards-defined PUS codecs. */
  class PusSecondaryHeaderFactory {
  public:
    [[nodiscard]] Result<std::shared_ptr<SecondaryHeaderAbstract>> create(
      PusRevision revision, PacketDirection direction, const MissionProfile &profile) const;
    [[nodiscard]] Result<std::shared_ptr<SecondaryHeaderAbstract>> create(
      const std::string &selector, const MissionProfile &profile) const;
    [[nodiscard]] bool typeIsSupported(const std::string &selector) const;
    [[nodiscard]] static bool isPusSelector(const std::string &selector) {
      return selector.rfind("PUS:", 0U) == 0U;
    }
  };

} // namespace CCSDS

#endif // PUS_SECONDARY_HEADER_FACTORY_H
