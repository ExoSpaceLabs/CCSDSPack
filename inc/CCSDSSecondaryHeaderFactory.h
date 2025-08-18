#ifndef CCSDS_SECONDARY_HEADER_FACTORY_H
#define CCSDS_SECONDARY_HEADER_FACTORY_H

#include <memory>
#include <unordered_map>
#include <functional>
#include <string>
#include "CCSDSSecondaryHeaderAbstract.h"  // Base class header

namespace CCSDS {

/**
 * @class SecondaryHeaderFactory
 * @brief A singleton factory class responsible for registering and creating instances of `SecondaryHeaderAbstract` objects.
 *
 * This factory allows clients to register new types of headers, check if a header type is registered, and create instances of registered header types.
 */
class SecondaryHeaderFactory {
public:
  /**
   * @brief Default constructor for the factory.
   */
  SecondaryHeaderFactory() = default;

  /**
   * @typedef CreatorFunc
   * @brief Type alias for a function that creates an instance of `SecondaryHeaderAbstract`.
   */
  using CreatorFunc = std::function<std::shared_ptr<SecondaryHeaderAbstract>()>;

  /**
   * @brief Registers a new header type with its creation function.
   *
   * This function adds a new header type to the factory by associating the header's type string with a shared pointer to the header.
   *
   * @param header A shared pointer to a `SecondaryHeaderAbstract` object to register.
     * @return ResultBool.
   */
  ResultBool registerType(std::shared_ptr<SecondaryHeaderAbstract> header) {
    RET_IF_ERR_MSG(!header, INVALID_HEADER_DATA, "Cannot register, invalid Header provided.");
    m_creators[header->getType()] = std::move(header);
    return true;
  }

  /**
   * @brief Creates an instance of a registered header type.
   *
   * This function searches for the header type by its string identifier and returns a new instance of the registered header.
   *
   * @param type A string representing the header type to create.
   * @return A shared pointer to a `SecondaryHeaderAbstract` object, or `nullptr` if the type is not registered.
   */
  std::shared_ptr<SecondaryHeaderAbstract> create(const std::string& type) {
    if (const auto it = m_creators.find(type); it != m_creators.end()) {
      return it->second; // Call the stored creation function
    }
    return nullptr; // Return nullptr if type not found
  }

  /**
   * @brief Checks if a header type is registered.
   *
   * This function checks if a given header type is already registered with the factory.
   *
   * @param type A string representing the header type to check.
   * @return `true` if the type is registered, `false` otherwise.
   */
  bool typeIsRegistered(const std::string& type) {
    if (const auto it = m_creators.find(type); it != m_creators.end()) {
      return true; // return true if found
    }
    return false; // Return false if type not found
  }

private:
  /**
   * @brief A map of header types to their corresponding `SecondaryHeaderAbstract` objects.
   *
   * This private member stores the registered header types along with their corresponding shared pointers.
   */
  std::unordered_map<std::string, std::shared_ptr<SecondaryHeaderAbstract> > m_creators;
};

} // namespace CCSDS

#endif // CCSDS_SECONDARY_HEADER_FACTORY_H
