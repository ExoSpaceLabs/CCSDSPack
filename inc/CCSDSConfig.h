// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSConfig.h
 * @brief Defines the host-only parser for CCSDSPack key:type=value configuration files.
 */
#ifndef CCSDS_CONFIG_H
#define CCSDS_CONFIG_H

#include <string>
#include <cstdint>
#include <vector>
#include "CCSDSResult.h"
#include <unordered_map>

/**
 * @class Config
 * @brief Parses and stores typed values from the CCSDSPack configuration format.
 *
 * Each non-empty configuration line uses the form `key:type=value`. Supported types
 * are string, int, float, bool, and byte vectors. Packet and Manager configuration
 * loaders retrieve values through get<T>() and return CONFIG_FILE_ERROR when required
 * keys or ranges are invalid.
 *
 * Config is excluded from CCSDS_MCU builds because it depends on hosted file and
 * string-processing facilities.
 */
class Config {
public:
  /** @brief Variant containing every value type supported by the parser. */
  using ConfigValue = std::variant<std::string, int, float, bool, std::vector<uint8_t>>;

  /**
   * @brief Loads and replaces configuration values from a file.
   * @param filename Path to a text configuration file.
   * @return Success, or FILE_READ_ERROR/CONFIG_FILE_ERROR for malformed input.
   */
  CCSDS::ResultBool load(const std::string &filename);

  /**
   * @brief Retrieves a value by exact key and requested type.
   * @tparam T One of ConfigValue's supported alternatives.
   * @param key Configuration key.
   * @return The copied value, NO_DATA when the key is absent, or INVALID_DATA for a type mismatch.
   */
  template<typename T>
  CCSDS::Result<T> get(const std::string& key) const {
    auto it = values.find(key);
    RET_IF_ERR_MSG(it == values.end(), CCSDS::ErrorCode::NO_DATA,
                   "Config: No data found for key: " + key);

    if (const auto* p = std::get_if<T>(&it->second)) {
      return *p;
    }
    return CCSDS::Error{CCSDS::ErrorCode::INVALID_DATA,
                        "Config: Wrong type for key: " + key};
  }

  /**
   * @brief Tests whether a key exists, regardless of its stored type.
   * @param key Configuration key.
   * @return True when the key is present.
   */
  bool isKey(const std::string& key) const;

private:
  std::unordered_map<std::string, ConfigValue> values; ///< Parsed values indexed by key.

  /**
   * @brief Splits one configuration line into key, type, and raw value fields.
   * @param line Input line.
   * @return Tuple containing key, type token, and unparsed value text.
   */
  static std::tuple<std::string, std::string, std::string> parseLine(const std::string& line);

  /**
   * @brief Parses a bracketed comma-separated byte vector.
   * @param valueStr Text such as `[1, 2, 255]`.
   * @return Parsed bytes, or CONFIG_FILE_ERROR for invalid syntax/range.
   */
  static CCSDS::ResultBuffer parseBytes(const std::string &valueStr);
};

#endif // CCSDS_CONFIG_H
