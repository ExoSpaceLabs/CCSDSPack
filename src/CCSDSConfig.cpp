#include "CCSDSConfig.h"
#include <cstddef>
#include <iomanip>
#include <algorithm>
#include <charconv>

#include <fstream>

//###########################################################################
#define VERBOSE 1

CCSDS::ResultBool Config::load(const std::string &filename) {
  std::ifstream file(filename);
  RET_IF_ERR_MSG(!file.is_open(),CCSDS::ErrorCode::CONFIG_FILE_ERROR, "Failed to open configuration file");

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line.front() == '#') continue;

    auto [key, type, valueStr] = parseLine(line);
    RET_IF_ERR_MSG(key.empty() || type.empty(),CCSDS::ErrorCode::CONFIG_FILE_ERROR, "Failed to parse configuration file");
    ConfigValue value;
    if (type == "string") {
      value = valueStr;
    } else if (type == "int") {
      std::uint8_t base = 10;
      if (valueStr.size() > 2 && valueStr.substr(0, 2) == "0x") {
        valueStr = valueStr.substr(2);
        base = 16;
      }
      value = std::stoi(valueStr, nullptr, base);
    } else if (type == "float") {
      value = std::stof(valueStr);
    } else if (type == "bool") {
      value = (valueStr == "true" || valueStr == "1");
    } else if (type == "bytes") {
      ASSIGN_CP( value, parseBytes(valueStr) );
    } else {
      return CCSDS::Error{CCSDS::ErrorCode::CONFIG_FILE_ERROR, " unknown type: " + type};
    }

    values[key] = value;
  }

  return true;
}

bool Config::isKey(const std::string &key) const {
  if (values.find(key) != values.end()) {
    return true;
  }
  return false;
}

std::tuple<std::string, std::string, std::string> Config::parseLine(const std::string& line) {
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

CCSDS::ResultBuffer Config::parseBytes(const std::string &valueStr) {
  std::vector<uint8_t> result{};
  RET_IF_ERR_MSG(valueStr.empty() || valueStr.front() != '[' || valueStr.back() != ']',
                 CCSDS::ErrorCode::CONFIG_FILE_ERROR,
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
      return CCSDS::Error{CCSDS::ErrorCode::CONFIG_FILE_ERROR, "Invalid byte value: <empty>"};
    }

    std::uint8_t base = 10;
    std::string_view sv{token};

    // Allow 0x / 0X prefix for hex
    if (sv.size() > 2 && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X')) {
      sv.remove_prefix(2);
      base = 16;
      if (sv.empty()) {
        return CCSDS::Error{CCSDS::ErrorCode::CONFIG_FILE_ERROR, "Invalid byte value: 0x"};
      }
    }

    // Parse without exceptions
    std::uint8_t tmp = 0;
    const char* first = sv.data();
    const char* last  = sv.data() + sv.size();
    auto res = std::from_chars(first, last, tmp, base);

    // Valid if parsed ok, consumed all characters, and fits in a byte
    if (res.ec != std::errc{} || res.ptr != last || tmp > 0xFFu) {
      return CCSDS::Error{CCSDS::ErrorCode::CONFIG_FILE_ERROR,
                          std::string("Invalid byte value: ") + std::string(token)};
    }
    result.push_back(static_cast<uint8_t>(tmp));
  }

  return result;
}
