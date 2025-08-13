#ifndef CCSDS_CONFIG_H
#define CCSDS_CONFIG_H

#include <string>
#include <cstdint>
#include <vector>
#include "CCSDSResult.h"

/// @brief Parses and stores config values from custom file format
class Config {
public:
  using ConfigValue = std::variant<std::string, int, float, bool, std::vector<uint8_t>>;

  /// @brief Load config file
  CCSDS::ResultBool load(const std::string &filename);

  /// @brief Get value by key and type
  template<typename T>
  CCSDS::Result<T> get(const std::string& key) const {
    auto it = values.find(key);
    RET_IF_ERR_MSG(it == values.end(), CCSDS::ErrorCode::NO_DATA,"Config: No data found for key: " + key);

    if (const auto* p = std::get_if<T>(&it->second)) {
      return *p;
    }
    return CCSDS::Error{ CCSDS::ErrorCode::INVALID_DATA, "Config: Wrong type for key: " + key};
  }

  bool isKey(const std::string& key) const;

private:
  std::unordered_map<std::string, ConfigValue> values;

  /// @brief Parse a single line from config
  static std::tuple<std::string, std::string, std::string> parseLine(const std::string& line);

  /// @brief Parse string "[1,2,3]" into vector<uint8_t>
  static CCSDS::ResultBuffer parseBytes(const std::string &valueStr);
};




#endif // CCSDS_CONFIG_H