// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "exec_utils.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

CCSDS::ResultBool parseArguments(
    const std::int32_t argc,
    char *argv[],
    std::unordered_map<std::string, std::string> &allowedMap,
    std::unordered_map<std::string, std::string> &outArgs,
    const std::set<std::string> &booleanArgs) {
  std::set<std::string> allowedKeys;
  std::set<std::string> allowedShortKeys;
  for (const auto &[shortKey, longKey] : allowedMap) {
    allowedShortKeys.insert(shortKey);
    allowedKeys.insert(longKey);
  }

  for (std::int32_t i = 1; i < argc; ++i) {
    const std::string current = argv[i];
    if (current.rfind("--", 0) != 0 && current.rfind('-', 0) != 0) {
      return CCSDS::Error{static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                          "Unknown argument: " + current};
    }

    std::string key;
    if (current.rfind("--", 0) == 0) {
      key = current.substr(2);
    } else {
      const std::string shortKey = current.substr(1);
      RET_IF_ERR_MSG(allowedShortKeys.find(shortKey) == allowedShortKeys.end(),
                     static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                     "Unknown argument: -" + shortKey);
      key = allowedMap.at(shortKey);
    }

    RET_IF_ERR_MSG(allowedKeys.find(key) == allowedKeys.end(),
                   static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                   "Unknown argument: --" + key);

    if (booleanArgs.find(key) != booleanArgs.end()) {
      outArgs[key] = "true";
      continue;
    }

    RET_IF_ERR_MSG(i + 1 >= argc,
                   static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                   "Missing value for argument: --" + key);
    outArgs[key] = argv[++i];
  }
  return true;
}

CCSDS::Result<CCSDS::PacketErrorControlMode>
parsePacketErrorControlMode(const std::string &value) {
  std::string normalized = value;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](const unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });

  if (normalized == "crc16") return CCSDS::PacketErrorControlMode::CRC16;
  if (normalized == "none") return CCSDS::PacketErrorControlMode::None;
  return CCSDS::Error{static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                      "Packet error control must be 'crc16' or 'none'"};
}

const char *packetErrorControlModeName(const CCSDS::PacketErrorControlMode mode) {
  return mode == CCSDS::PacketErrorControlMode::CRC16 ? "crc16" : "none";
}

CCSDS::Result<PacketStreamLayout>
inspectPacketStream(const std::vector<std::uint8_t> &data,
                    const bool syncPatternEnable,
                    const std::uint32_t syncPattern,
                    const bool allowTrailingBytes) {
  PacketStreamLayout layout;
  std::size_t offset{0U};

  const auto leaveTrailing = [&](const std::size_t frameOffset) {
    return allowTrailingBytes && !layout.packets.empty() && frameOffset < data.size();
  };

  while (offset < data.size()) {
    const std::size_t frameOffset = offset;

    if (syncPatternEnable) {
      if (data.size() - offset < 4U) {
        if (leaveTrailing(frameOffset)) break;
        return CCSDS::Error{CCSDS::ErrorCode::INVALID_DATA,
                            "Truncated packet synchronization marker at byte "
                              + std::to_string(frameOffset)};
      }

      const std::uint32_t encoded =
        (static_cast<std::uint32_t>(data[offset]) << 24U)
        | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U)
        | (static_cast<std::uint32_t>(data[offset + 2U]) << 8U)
        | static_cast<std::uint32_t>(data[offset + 3U]);
      if (encoded != syncPattern) {
        if (leaveTrailing(frameOffset)) break;
        return CCSDS::Error{CCSDS::ErrorCode::INVALID_DATA,
                            "Packet synchronization marker mismatch at byte "
                              + std::to_string(frameOffset)};
      }
      offset += 4U;
    }

    if (data.size() - offset < 6U) {
      if (leaveTrailing(frameOffset)) {
        offset = frameOffset;
        break;
      }
      return CCSDS::Error{CCSDS::ErrorCode::INVALID_HEADER_DATA,
                          "Truncated CCSDS primary header at byte "
                            + std::to_string(offset)};
    }

    const std::vector<std::uint8_t> headerBytes(
      data.begin() + static_cast<std::ptrdiff_t>(offset),
      data.begin() + static_cast<std::ptrdiff_t>(offset + 6U));
    CCSDS::Header header;
    const auto headerResult = header.deserialize(headerBytes);
    if (!headerResult) {
      if (leaveTrailing(frameOffset)) {
        offset = frameOffset;
        break;
      }
      return headerResult.error();
    }

    const std::size_t packetSize =
      6U + static_cast<std::size_t>(header.getDataLength()) + 1U;
    if (data.size() - offset < packetSize) {
      if (leaveTrailing(frameOffset)) {
        offset = frameOffset;
        break;
      }
      return CCSDS::Error{CCSDS::ErrorCode::INVALID_DATA,
                          "Packet Data Length declares "
                            + std::to_string(packetSize)
                            + " bytes at offset " + std::to_string(offset)
                            + ", but only " + std::to_string(data.size() - offset)
                            + " bytes remain"};
    }

    layout.packets.push_back({offset, packetSize});
    offset += packetSize;
    layout.consumedBytes = offset;
  }

  return layout;
}

void customConsole(const std::string &appName, const std::string &message,
                   const std::string &logLevel) {
  const auto now = std::chrono::high_resolution_clock::now();
  const auto duration = now.time_since_epoch();
  const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
  const std::time_t currentTime =
    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const std::tm *tm = std::localtime(&currentTime);

  std::ostringstream timestampStream;
  timestampStream << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
  std::cout << "[" << timestampStream.str() << "."
            << std::setw(6) << std::setfill('0') << microseconds.count() % 1000000
            << "] [" << appName << "] [" << logLevel << "] : "
            << message << std::endl;
}
