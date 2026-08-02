// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/** @file exec_encoder.cpp @brief Command-line encoder for CCSDS Space Packet streams. */

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "CCSDSPack.h"
#include "exec_utils.h"

namespace {
void printHelp() {
  std::cout
    << "Usage: ccsds_encoder [OPTIONS]\n"
    << "Encode one input file into one or more CCSDS Space Packets.\n\n"
    << "Mandatory parameters:\n"
    << "  -i, --input <filename>                 Input application-data file\n"
    << "  -o, --output <filename>                Output packet-stream file\n"
    << "  -c, --config <filename>                Packet template configuration\n\n"
    << "Optional parameters:\n"
    << "  -e, --packet-error-control <mode>      crc16 or none; overrides config\n"
    << "                                           (default: config, otherwise crc16)\n"
    << "  -v, --verbose                         Print generated packet information\n"
    << "  -h, --help                            Show this help message\n\n"
    << "The configuration file defines the packet template, data-field capacity,\n"
    << "and optional synchronization marker. Packet Data Length and CRC16 are\n"
    << "calculated by the v1.2 packet serializer.\n";
}

int printError(const CCSDS::Error &error) {
  std::cerr << "[ Error " << static_cast<unsigned>(error.code()) << " ]: "
            << error.message() << std::endl;
  return static_cast<int>(error.code());
}

CCSDS::ResultBool configureManagerFraming(CCSDS::Manager &manager, const Config &cfg) {
  if (!cfg.isKey("sync_pattern_enable")) return true;

  bool enabled{};
  ASSIGN_CP(enabled, cfg.get<bool>("sync_pattern_enable"));
  manager.setSyncPatternEnable(enabled);
  if (enabled && cfg.isKey("sync_pattern")) {
    int encoded{};
    ASSIGN_CP(encoded, cfg.get<int>("sync_pattern"));
    manager.setSyncPattern(static_cast<std::uint32_t>(encoded));
  }
  return true;
}
}

int main(const int argc, char *argv[]) {
  const std::string appName = "ccsds_encoder";
  std::unordered_map<std::string, std::string> allowed{
    {"h", "help"}, {"v", "verbose"}, {"i", "input"},
    {"o", "output"}, {"c", "config"}, {"e", "packet-error-control"}
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
    printHelp();
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
    if (!result) { printHelp(); return printError(result.error()); }
    input = result.value();
  }
  {
    auto result = requirePath("output", "Output file");
    if (!result) { printHelp(); return printError(result.error()); }
    output = result.value();
  }
  {
    auto result = requirePath("config", "Config file");
    if (!result) { printHelp(); return printError(result.error()); }
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
  if (const auto result = configureManagerFraming(manager, cfg); !result) {
    return printError(result.error());
  }

  customConsole(appName,
                std::string("packet error control: ")
                  + packetErrorControlModeName(manager.getTemplate().getPacketErrorControlMode()));

  std::vector<std::uint8_t> inputBytes;
  customConsole(appName, "reading data from " + input);
  {
    const auto result = readBinaryFile(input);
    if (!result) return printError(result.error());
    inputBytes = result.value();
  }

  if (manager.getTemplate().getPrimaryHeader().getSequenceFlags() == CCSDS::UNSEGMENTED
      && inputBytes.size() > manager.getDataFieldSize()) {
    return printError(CCSDS::Error{
      static_cast<CCSDS::ErrorCode>(INVALID_INPUT_DATA),
      "Input contains " + std::to_string(inputBytes.size())
        + " bytes, exceeding the unsegmented packet capacity of "
        + std::to_string(manager.getDataFieldSize())});
  }

  customConsole(appName, "generating CCSDS packets using input data");
  if (const auto result = manager.setApplicationData(inputBytes); !result) {
    return printError(result.error());
  }

  if (args["verbose"] == "true") printPackets(manager);

  const auto packets = manager.getPacketsBuffer();
  if (!manager.getPackets().empty() && packets.empty()) {
    return printError(CCSDS::Error{CCSDS::ErrorCode::INVALID_HEADER_DATA,
                                   "Unable to serialize generated packet stream"});
  }

  customConsole(appName, "writing data to " + output);
  if (const auto result = writeBinaryFile(packets, output); !result) {
    return printError(result.error());
  }

  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::high_resolution_clock::now() - start);
  customConsole(appName, "execution time: " + std::to_string(duration.count()) + " [us]");
  customConsole(appName, "[ Exit code 0 ]");
  return 0;
}
