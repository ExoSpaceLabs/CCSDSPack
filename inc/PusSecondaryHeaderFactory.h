// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef PUS_SECONDARY_HEADER_FACTORY_H
#define PUS_SECONDARY_HEADER_FACTORY_H

#include "PusSecondaryHeaders.h"
#include <memory>
#include <string>

namespace ccsds::pus {

  /** @brief Fixed, non-extensible registry for standards-defined PUS codecs. */
  class SecondaryHeaderFactory {
  public:
    /** @brief Creates a default-tailored PUS codec from revision and packet direction. */
    [[nodiscard]] Result<std::shared_ptr<SecondaryHeaderAbstract>> create(
      Revision revision, PacketDirection direction) const;

    /** @brief Creates a default-tailored PUS codec from its canonical selector. */
    [[nodiscard]] Result<std::shared_ptr<SecondaryHeaderAbstract>> create(
      const std::string &selector) const;

    /** @brief Clones one concrete PUS header, retaining its tailoring and current values. */
    [[nodiscard]] Result<std::shared_ptr<SecondaryHeaderAbstract>> clone(
      const SecondaryHeader &header) const;

    /** @brief Returns whether a canonical PUS selector is supported. */
    [[nodiscard]] bool typeIsSupported(const std::string &selector) const;

    /** @brief Returns whether a selector belongs to the reserved PUS namespace. */
    [[nodiscard]] static bool isPusSelector(const std::string &selector) {
      return selector.rfind("PUS:", 0U) == 0U;
    }
  };

} // namespace ccsds::pus

#endif // PUS_SECONDARY_HEADER_FACTORY_H
