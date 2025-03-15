#ifndef HEADER_FACTORY_H
#define HEADER_FACTORY_H

#include <memory>
#include <unordered_map>
#include <functional>
#include <string>
#include "CCSDSSecondaryHeaderAbstract.h"  // Base class header

namespace CCSDS {
class SecondaryHeaderFactory {
public:
  using CreatorFunc = std::function<std::shared_ptr<SecondaryHeaderAbstract>()>;

  static SecondaryHeaderFactory& instance() {
    static SecondaryHeaderFactory factory;
    return factory;
  }

  // Register a new header type with its creation function
  void registerType(std::shared_ptr<SecondaryHeaderAbstract> header) {
    m_creators[header->getType()] = std::move(header);
  }

  // Create an instance of a registered header type
  std::shared_ptr<SecondaryHeaderAbstract> create(const std::string& type) {
    auto it = m_creators.find(type);
    if (it != m_creators.end()) {
      return it->second; // Call the stored creation function
    }
    return nullptr; // Return nullptr if type not found
  }

  bool typeIsRegistered(const std::string& type) {
    auto it = m_creators.find(type);
    if (it != m_creators.end()) {
      return true; // return true if found
    }
    return false; // Return false if type not found
  }

private:
  std::unordered_map<std::string, std::shared_ptr<SecondaryHeaderAbstract> > m_creators;

  // Private constructor to enforce singleton pattern
  SecondaryHeaderFactory() = default;
};
}

#endif // HEADER_FACTORY_H
