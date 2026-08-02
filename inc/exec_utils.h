// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef EXEC_UTILS_H
#define EXEC_UTILS_H

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "CCSDSPacket.h"
#include "CCSDSResult.h"

enum ErrorCodeExec : std::uint8_t {
  ARG_PARSE_ERROR = 14,             ///< Error parsing a command-line argument.
  CONFIG_MISSING_PARAMETER = 15,    ///< Required CLI configuration value is absent.
  INVALID_INPUT_DATA = 16,          ///< Input does not contain a usable packet or payload.
  OTHER = 17,                       ///< Unclassified executable failure.
  PACKET_VALIDATION_FAILED = 18     ///< At least one packet validation check failed.
};

/** @brief Byte range occupied by one CCSDS packet inside a larger stream. */
struct PacketStreamSlice {
  std::size_t offset{}; ///< Offset of the six-byte primary header, excluding an optional sync marker.
  std::size_t size{};   ///< Complete CCSDS packet size, including optional packet error control.
};

/** @brief Result of scanning adjacent CCSDS packets by their encoded Packet Data Length. */
struct PacketStreamLayout {
  std::vector<PacketStreamSlice> packets{}; ///< Complete packets found in order.
  std::size_t consumedBytes{};              ///< Prefix occupied by packets and optional sync markers.
};

/**
 * @brief Parses supported short and long command-line options.
 * @param argc Argument count supplied to main().
 * @param argv Argument vector supplied to main().
 * @param allowedMap Mapping from short option names to long option names.
 * @param outArgs Parsed long-name/value map.
 * @param booleanArgs Long option names which do not consume a value.
 * @return Success, or ARG_PARSE_ERROR for unknown options or missing values.
 */
CCSDS::ResultBool parseArguments(int argc, char *argv[],
                                 std::unordered_map<std::string, std::string> &allowedMap,
                                 std::unordered_map<std::string, std::string> &outArgs,
                                 const std::set<std::string> &booleanArgs);

/**
 * @brief Parses a CLI packet-error-control mode.
 * @param value Case-insensitive `crc16` or `none`.
 * @return Parsed mode, or ARG_PARSE_ERROR for an unsupported value.
 */
CCSDS::Result<CCSDS::PacketErrorControlMode>
parsePacketErrorControlMode(const std::string &value);

/** @brief Returns the stable CLI spelling for a packet-error-control mode. */
const char *packetErrorControlModeName(CCSDS::PacketErrorControlMode mode);

/**
 * @brief Scans adjacent packets using each encoded Packet Data Length boundary.
 * @param data Input stream.
 * @param syncPatternEnable Whether every packet is prefixed by a four-byte sync marker.
 * @param syncPattern Expected big-endian sync marker.
 * @param allowTrailingBytes When true, a non-packet suffix after at least one packet is left unconsumed.
 * @return Packet ranges and consumed prefix, or a deterministic framing/length error.
 */
CCSDS::Result<PacketStreamLayout>
inspectPacketStream(const std::vector<std::uint8_t> &data,
                    bool syncPatternEnable,
                    std::uint32_t syncPattern,
                    bool allowTrailingBytes);

/**
 * @brief Prints a timestamped executable log message.
 * @param appName Executable name.
 * @param message Message text.
 * @param logLevel Displayed severity label.
 */
void customConsole(const std::string &appName, const std::string &message,
                   const std::string &logLevel = "INFO");

#endif // EXEC_UTILS_H
