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

  SecondaryHeaderFactory() = default;

  using CreatorFunc = std::function<std::shared_ptr<SecondaryHeaderAbstract>()>;

  // Register a new header type with its creation function
  void registerType(std::shared_ptr<SecondaryHeaderAbstract> header) {
    m_creators[header->getType()] = std::move(header);
  }

  // Create an instance of a registered header type
  std::shared_ptr<SecondaryHeaderAbstract> create(const std::string& type) {
    if (const auto it = m_creators.find(type); it != m_creators.end()) {
      return it->second; // Call the stored creation function
    }
    return nullptr; // Return nullptr if type not found
  }

  bool typeIsRegistered(const std::string& type) {
    if (const auto it = m_creators.find(type); it != m_creators.end()) {
      return true; // return true if found
    }
    return false; // Return false if type not found
  }

private:
  std::unordered_map<std::string, std::shared_ptr<SecondaryHeaderAbstract> > m_creators;
};
}

#endif // HEADER_FACTORY_H
