// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSSecondaryHeaderFactory.h
 * @brief Defines the per-DataField registry used to look up secondary-header implementations.
 */
#ifndef CCSDS_SECONDARY_HEADER_FACTORY_H
#define CCSDS_SECONDARY_HEADER_FACTORY_H

#include <memory>
#include <unordered_map>
#include <functional>
#include <string>
#include "CCSDSSecondaryHeaderAbstract.h"

namespace CCSDS {

/**
 * @class SecondaryHeaderFactory
 * @brief Registers secondary-header prototypes by type name and returns their shared instances.
 *
 * Each DataField owns one factory. registerType() stores the supplied shared pointer
 * under header->getType(). create() returns that same stored shared pointer; it does
 * not clone or default-construct a fresh object. Consequently, callers that need
 * independent mutable header instances should register separate objects in separate
 * DataField/Packet instances or explicitly supply their own shared object.
 *
 * Re-registering an existing type replaces the previous stored pointer. The class is
 * not synchronized for concurrent registration or lookup.
 */
class SecondaryHeaderFactory {
public:
  /** @brief Constructs an empty registry. */
  SecondaryHeaderFactory() = default;

  /**
   * @typedef CreatorFunc
   * @brief Legacy alias for a function returning a shared secondary-header object.
   * @note The current registry stores shared objects directly rather than CreatorFunc values.
   */
  using CreatorFunc = std::function<std::shared_ptr<SecondaryHeaderAbstract>()>;

  /**
   * @brief Registers or replaces a secondary-header object by its getType() key.
   * @param header Shared object to store.
   * @return Success, or INVALID_HEADER_DATA when header is nullptr.
   * @note Ownership is shared with the caller; later mutations are visible through the registry.
   */
  ResultBool registerType(std::shared_ptr<SecondaryHeaderAbstract> header) {
    RET_IF_ERR_MSG(!header, INVALID_HEADER_DATA, "Cannot register, invalid Header provided.");
    m_creators[header->getType()] = std::move(header);
    return true;
  }

  /**
   * @brief Looks up a registered secondary-header object.
   * @param type Exact string returned by SecondaryHeaderAbstract::getType().
   * @return The stored shared pointer, or nullptr when the type is unknown.
   * @warning The returned object is the registry's stored instance, not a clone.
   */
  std::shared_ptr<SecondaryHeaderAbstract> create(const std::string& type) {
    if (const auto it = m_creators.find(type); it != m_creators.end()) {
      return it->second;
    }
    return nullptr;
  }

  /**
   * @brief Tests whether a type key is present in the registry.
   * @param type Exact secondary-header type name.
   * @return True when a shared object is registered under the key.
   */
  bool typeIsRegistered(const std::string& type) {
    if (const auto it = m_creators.find(type); it != m_creators.end()) {
      return true;
    }
    return false;
  }

private:
  /** @brief Registered type names mapped to shared mutable header objects. */
  std::unordered_map<std::string, std::shared_ptr<SecondaryHeaderAbstract> > m_creators;
};

} // namespace CCSDS

#endif // CCSDS_SECONDARY_HEADER_FACTORY_H
