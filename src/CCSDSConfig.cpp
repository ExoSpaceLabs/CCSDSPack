// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSConfig.h"
#include <cstddef>
#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <charconv>

#include <fstream>
#include <utility>

//###########################################################################
#define VERBOSE 1

ccsds::ResultBool ccsds::Config::load(const std::string &filename) {
  std::ifstream file(filename);
  RET_IF_ERR_MSG(!file.is_open(),ccsds::ErrorCode::CONFIG_FILE_ERROR, "Failed to open configuration file");

  std::unordered_map<std::string, ConfigValue> parsedValues;
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line.front() == '#') continue;

    auto [key, type, valueStr] = parseLine(line);
    RET_IF_ERR_MSG(key.empty() || type.empty(),ccsds::ErrorCode::CONFIG_FILE_ERROR, "Failed to parse configuration file");
    ConfigValue value;
    if (type == "string") {
      value = valueStr;
    } else if (type == "int" || type == "uint") {
      const bool negative = !valueStr.empty() && valueStr.front() == '-';
      std::string_view encoded{valueStr};
      std::uint8_t base = 10U;
      if (negative) encoded.remove_prefix(1U);
      if (encoded.size() > 2U && encoded[0] == '0'
          && (encoded[1] == 'x' || encoded[1] == 'X')) {
        encoded.remove_prefix(2U);
        base = 16U;
      }
      RET_IF_ERR_MSG(encoded.empty(), ccsds::ErrorCode::CONFIG_FILE_ERROR,
                     "Config: Invalid integer value for key: " + key);

      std::uint64_t parsed{};
      const auto conversion = std::from_chars(encoded.data(),
                                               encoded.data() + encoded.size(),
                                               parsed, base);
      RET_IF_ERR_MSG(conversion.ec != std::errc{}
                     || conversion.ptr != encoded.data() + encoded.size(),
                     ccsds::ErrorCode::CONFIG_FILE_ERROR,
                     "Config: Invalid integer value for key: " + key);
      if (type == "uint") {
        RET_IF_ERR_MSG(negative, ccsds::ErrorCode::CONFIG_FILE_ERROR,
                       "Config: Unsigned value cannot be negative for key: " + key);
        value = parsed;
      } else {
        constexpr auto positiveLimit = static_cast<std::uint64_t>(INT32_MAX);
        constexpr auto negativeLimit = positiveLimit + 1U;
        RET_IF_ERR_MSG((!negative && parsed > positiveLimit)
                       || (negative && parsed > negativeLimit),
                       ccsds::ErrorCode::CONFIG_FILE_ERROR,
                       "Config: Integer value is out of range for key: " + key);
        value = negative ? static_cast<int>(-static_cast<std::int64_t>(parsed))
                         : static_cast<int>(parsed);
      }
    } else if (type == "float") {
      errno = 0;
      char *end{};
      const float parsed = std::strtof(valueStr.c_str(), &end);
      RET_IF_ERR_MSG(errno == ERANGE || end == valueStr.c_str() || *end != '\0',
                     ccsds::ErrorCode::CONFIG_FILE_ERROR,
                     "Config: Invalid float value for key: " + key);
      value = parsed;
    } else if (type == "bool") {
      RET_IF_ERR_MSG(valueStr != "true" && valueStr != "false"
                     && valueStr != "1" && valueStr != "0",
                     ccsds::ErrorCode::CONFIG_FILE_ERROR,
                     "Config: Invalid bool value for key: " + key);
      value = valueStr == "true" || valueStr == "1";
    } else if (type == "bytes") {
      ASSIGN_CP( value, parseBytes(valueStr) );
    } else {
      return ccsds::Error{ccsds::ErrorCode::CONFIG_FILE_ERROR, " unknown type: " + type};
    }

    parsedValues[key] = value;
  }

  values = std::move(parsedValues);
  return true;
}

bool ccsds::Config::isKey(const std::string &key) const {
  if (values.find(key) != values.end()) {
    return true;
  }
  return false;
}

std::tuple<std::string, std::string, std::string> ccsds::Config::parseLine(
    const std::string& line) {
  auto colonPos = line.find(':');
  auto equalPos = line.find('=');

  if (colonPos == std::string::npos || equalPos == std::string::npos || equalPos < colonPos)
    return {"", "", ""};

  std::string key = line.substr(0, colonPos);
  std::string type = line.substr(colonPos + 1, equalPos - colonPos - 1);
  std::string value = line.substr(equalPos + 1);

  if (!value.empty() && value.front() == '"' && value.back() == '"')
    value = value.substr(1, value.size() - 2);

  return {key, type, value};
}

ccsds::ResultBuffer ccsds::Config::parseBytes(const std::string &valueStr) {
  std::vector<uint8_t> result{};
  RET_IF_ERR_MSG(valueStr.empty() || valueStr.front() != '[' || valueStr.back() != ']',
                 ccsds::ErrorCode::CONFIG_FILE_ERROR,
                 "Config: Invalid buffer formatting []");

  if (valueStr == "[]" || valueStr == "[ ]") {
    return result;
  }

  // Strip surrounding [ ... ]
  std::string inner = valueStr.substr(1, valueStr.size() - 2);
  std::stringstream ss(inner);
  std::string token;

  while (std::getline(ss, token, ',')) {
    // Remove all spaces inside each token
    token.erase(std::remove_if(token.begin(), token.end(),
                               [](unsigned char c){ return std::isspace(c); }),
                token.end());

    // Empty token after trimming is invalid (e.g., "[12, ,34]")
    if (token.empty()) {
      return ccsds::Error{ccsds::ErrorCode::CONFIG_FILE_ERROR, "Invalid byte value: <empty>"};
    }

    std::uint8_t base = 10;
    std::string_view sv{token};

    // Allow 0x / 0X prefix for hex
    if (sv.size() > 2 && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X')) {
      sv.remove_prefix(2);
      base = 16;
      if (sv.empty()) {
        return ccsds::Error{ccsds::ErrorCode::CONFIG_FILE_ERROR, "Invalid byte value: 0x"};
      }
    }

    // Parse without exceptions
    unsigned int tmp = 0U;
    const char* first = sv.data();
    const char* last  = sv.data() + sv.size();
    auto res = std::from_chars(first, last, tmp, base);

    // Valid if parsed ok, consumed all characters, and fits in a byte
    if (res.ec != std::errc{} || res.ptr != last || tmp > 0xFFu) {
      return ccsds::Error{ccsds::ErrorCode::CONFIG_FILE_ERROR,
                          std::string("Invalid byte value: ") + std::string(token)};
    }
    result.push_back(static_cast<uint8_t>(tmp));
  }

  return result;
}
