// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/** @file exec_decoder.cpp @brief Command-line decoder for CCSDS Space Packet streams. */

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "CCSDSPack.h"
#include "exec_utils.h"

namespace {
void printHelpDecoder() {
  std::cout
    << "Usage: ccsds_decoder [OPTIONS]\n"
    << "Decode adjacent CCSDS Space Packets and reassemble application data.\n\n"
    << "Mandatory parameters:\n"
    << "  -i, --input <filename>                 Input packet-stream file\n"
    << "  -o, --output <filename>                Reassembled application-data file\n"
    << "  -c, --config <filename>                Packet template configuration\n\n"
    << "Optional parameters:\n"
    << "  -e, --packet-error-control <mode>      crc16 or none; overrides config\n"
    << "                                           (default: config, otherwise crc16)\n"
    << "  -t, --trailing-output <filename>       Save bytes after the last complete packet\n"
    << "  -v, --verbose                         Print decoded packet information\n"
    << "  -h, --help                            Show this help message\n\n"
    << "Packet boundaries are taken from each encoded Packet Data Length field.\n"
    << "A suffix that cannot form another complete packet is left unconsumed and\n"
    << "can be saved with --trailing-output. Use ccsds_validator for strict stream checks.\n";
}

int printError(const CCSDS::Error &error) {
  std::cerr << "[ Error " << static_cast<unsigned>(error.code()) << " ]: "
            << error.message() << std::endl;
  return static_cast<int>(error.code());
}

struct DecoderConfig {
  bool validationEnable{};
  bool syncPatternEnable{};
  std::uint32_t syncPattern{0x1ACFFC1DU};
};

CCSDS::Result<DecoderConfig> readDecoderConfig(const Config &cfg) {
  DecoderConfig settings;
  RET_IF_ERR_MSG(!cfg.isKey("validation_enable"),
                 static_cast<CCSDS::ErrorCode>(CONFIG_MISSING_PARAMETER),
                 "Config: Missing bool field: validation_enable");
  ASSIGN_CP(settings.validationEnable, cfg.get<bool>("validation_enable"));

  if (cfg.isKey("sync_pattern_enable")) {
    ASSIGN_CP(settings.syncPatternEnable, cfg.get<bool>("sync_pattern_enable"));
  }
  if (settings.syncPatternEnable && cfg.isKey("sync_pattern")) {
    int encoded{};
    ASSIGN_CP(encoded, cfg.get<int>("sync_pattern"));
    settings.syncPattern = static_cast<std::uint32_t>(encoded);
  }
  return settings;
}
}

int main(const int argc, char *argv[]) {
  const std::string appName = "ccsds_decoder";
  std::unordered_map<std::string, std::string> allowed{
    {"h", "help"}, {"v", "verbose"}, {"i", "input"},
    {"o", "output"}, {"c", "config"}, {"e", "packet-error-control"},
    {"t", "trailing-output"}
  };
  const std::set<std::string> booleanArgs{"verbose", "help"};
  std::unordered_map<std::string, std::string> args{
    {"verbose", "false"}, {"help", "false"}
  };

  const auto start = std::chrono::high_resolution_clock::now();
  if (const auto result = parseArguments(argc, argv, allowed, args, booleanArgs); !result) {
    return printError(result.error());
  }
  if (args["help"] == "true") {
    printHelpDecoder();
    return 0;
  }

  const auto requirePath = [&](const char *key, const char *description)
      -> CCSDS::Result<std::string> {
    const auto it = args.find(key);
    if (it == args.end() || it->second.empty()) {
      return CCSDS::Error{static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                          std::string(description) + " must be specified"};
    }
    return it->second;
  };

  std::string input;
  std::string output;
  std::string configFile;
  {
    auto result = requirePath("input", "Input file");
    if (!result) { printHelpDecoder(); return printError(result.error()); }
    input = result.value();
  }
  {
    auto result = requirePath("output", "Output file");
    if (!result) { printHelpDecoder(); return printError(result.error()); }
    output = result.value();
  }
  {
    auto result = requirePath("config", "Config file");
    if (!result) { printHelpDecoder(); return printError(result.error()); }
    configFile = result.value();
  }

  if (!fileExists(input)) {
    return printError(CCSDS::Error{static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                                   "Input file does not exist: " + input});
  }
  if (!fileExists(configFile)) {
    return printError(CCSDS::Error{static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                                   "Config file does not exist: " + configFile});
  }

  customConsole(appName, "reading CCSDS configuration file: " + configFile);
  Config cfg;
  if (const auto result = cfg.load(configFile); !result) return printError(result.error());

  DecoderConfig settings;
  {
    const auto result = readDecoderConfig(cfg);
    if (!result) return printError(result.error());
    settings = result.value();
  }

  CCSDS::Packet templatePacket;
  if (const auto result = templatePacket.loadFromConfig(cfg); !result) {
    return printError(result.error());
  }
  if (const auto it = args.find("packet-error-control"); it != args.end()) {
    const auto result = parsePacketErrorControlMode(it->second);
    if (!result) return printError(result.error());
    templatePacket.setPacketErrorControlMode(result.value());
  }

  CCSDS::Manager manager;
  if (const auto result = manager.setPacketTemplate(std::move(templatePacket)); !result) {
    return printError(result.error());
  }
  manager.setSyncPattern(settings.syncPattern);
  manager.setSyncPatternEnable(settings.syncPatternEnable);
  manager.setAutoValidateEnable(false);

  std::vector<std::uint8_t> inputBytes;
  customConsole(appName, "reading data from " + input);
  {
    const auto result = readBinaryFile(input);
    if (!result) return printError(result.error());
    inputBytes = result.value();
  }

  PacketStreamLayout layout;
  {
    const auto result = inspectPacketStream(inputBytes, settings.syncPatternEnable,
                                            settings.syncPattern, true);
    if (!result) return printError(result.error());
    layout = result.value();
  }
  if (layout.packets.empty()) {
    return printError(CCSDS::Error{static_cast<CCSDS::ErrorCode>(INVALID_INPUT_DATA),
                                   "Input does not begin with a complete CCSDS packet"});
  }

  const std::vector<std::uint8_t> packetPrefix(
    inputBytes.begin(),
    inputBytes.begin() + static_cast<std::ptrdiff_t>(layout.consumedBytes));
  const std::vector<std::uint8_t> trailing(
    inputBytes.begin() + static_cast<std::ptrdiff_t>(layout.consumedBytes),
    inputBytes.end());

  customConsole(appName,
                "decoding " + std::to_string(layout.packets.size())
                  + " packet(s) using "
                  + packetErrorControlModeName(manager.getTemplate().getPacketErrorControlMode())
                  + " packet error control");
  if (const auto result = manager.load(packetPrefix); !result) {
    return printError(result.error());
  }

  if (!trailing.empty()) {
    customConsole(appName,
                  "leaving " + std::to_string(trailing.size())
                    + " trailing byte(s) unconsumed", "WARN");
  }
  if (const auto it = args.find("trailing-output"); it != args.end()) {
    if (const auto result = writeBinaryFile(trailing, it->second); !result) {
      return printError(result.error());
    }
  }

  if (args["verbose"] == "true") printPackets(manager);

  manager.setAutoValidateEnable(settings.validationEnable);
  std::vector<std::uint8_t> outputData;
  {
    const auto result = manager.getApplicationDataBuffer();
    if (!result) return printError(result.error());
    outputData = result.value();
  }

  customConsole(appName, "writing data to " + output);
  if (const auto result = writeBinaryFile(outputData, output); !result) {
    return printError(result.error());
  }

  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::high_resolution_clock::now() - start);
  customConsole(appName, "execution time: " + std::to_string(duration.count()) + " [us]");
  customConsole(appName, "[ Exit code 0 ]");
  return 0;
}
