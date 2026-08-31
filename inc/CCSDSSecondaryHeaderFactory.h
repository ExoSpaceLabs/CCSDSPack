// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDS_SECONDARY_HEADER_FACTORY_H
#define CCSDS_SECONDARY_HEADER_FACTORY_H

#include "CCSDSSecondaryHeaderAbstract.h"
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace ccsds {

  /** Direction-neutral extension registry for mission-specific secondary headers. */
  class SecondaryHeaderFactory {
  public:
    using CreatorFunc = std::function<std::shared_ptr<SecondaryHeaderAbstract>()>;

    template <typename T>
    ResultBool registerType() {
      static_assert(std::is_base_of<SecondaryHeaderAbstract, T>::value,
                    "T must derive from SecondaryHeaderAbstract");
      static_assert(std::is_default_constructible<T>::value,
                    "Custom secondary-header types must be default constructible");
      const auto probe = std::make_shared<T>();
      return registerCreator(probe->getType(), [] {
        return std::static_pointer_cast<SecondaryHeaderAbstract>(std::make_shared<T>());
      });
    }

    ResultBool registerCreator(const std::string &type, CreatorFunc creator) {
      RET_IF_ERR_MSG(type.empty(), ErrorCode::INVALID_HEADER_DATA,
                     "Cannot register an empty secondary-header type.");
      RET_IF_ERR_MSG(type.rfind("PUS:", 0U) == 0U, ErrorCode::INVALID_HEADER_DATA,
                     "The PUS: namespace is reserved for standards-defined codecs.");
      RET_IF_ERR_MSG(!creator, ErrorCode::INVALID_HEADER_DATA,
                     "Cannot register an empty secondary-header creator.");
      RET_IF_ERR_MSG(m_creators.find(type) != m_creators.end(), ErrorCode::INVALID_HEADER_DATA,
                     "Secondary-header type is already registered: " + type);
      m_creators.emplace(type, std::move(creator));
      return true;
    }

    [[nodiscard]] std::shared_ptr<SecondaryHeaderAbstract> create(const std::string &type) const {
      const auto it = m_creators.find(type);
      return it == m_creators.end() ? nullptr : it->second();
    }

    [[nodiscard]] bool typeIsRegistered(const std::string &type) const {
      return m_creators.find(type) != m_creators.end();
    }

  private:
    std::unordered_map<std::string, CreatorFunc> m_creators;
  };

} // namespace ccsds

#endif // CCSDS_SECONDARY_HEADER_FACTORY_H
