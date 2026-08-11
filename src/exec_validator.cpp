// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/** @file exec_validator.cpp @brief Command-line validator for CCSDS Space Packet streams. */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "CCSDSPack.h"
#include "exec_utils.h"

namespace {

void printHelp() {
  std::cout
    << "Usage: ccsds_validator [OPTIONS]\n"
    << "Validate one or more adjacent CCSDS Space Packets.\n\n"
    << "Mandatory parameters:\n"
    << "  -i, --input <filename>                 Input packet-stream file\n\n"
    << "Optional parameters:\n"
    << "  -c, --config <filename>                Template/framing configuration\n"
    << "  -e, --packet-error-control <mode>      crc16 or none; overrides config\n"
    << "                                           (default: config, otherwise crc16)\n"
    << "  -v, --verbose                          Print every performed validation check\n"
    << "  -p, --print-packets                    Print packets that parse successfully\n"
    << "  -h, --help                             Show this help message\n\n"
    << "The configured Packet template is also the secondary-header parsing schema.\n"
    << "PUS revision/direction come from the concrete PUS header type; optional PUS\n"
    << "tailoring comes from that header. Packet error control remains packet-level.\n";
}

int printError(const ccsds::Error &error) {
  std::cerr << "[ Error " << static_cast<unsigned>(error.code()) << " ]: "
            << error.message() << std::endl;
  return static_cast<int>(error.code());
}

struct ValidatorSettings {
  ccsds::PacketErrorControlMode mode{ccsds::PacketErrorControlMode::CRC16};
  bool syncPatternEnable{};
  std::uint32_t syncPattern{0x1ACFFC1DU};
  bool hasTemplate{};
  ccsds::Packet templatePacket{};
};

ccsds::Result<ValidatorSettings>
loadValidatorSettings(const std::unordered_map<std::string, std::string> &args) {
  ValidatorSettings settings;

  const auto configIt = args.find("config");
  if (configIt != args.end()) {
    if (!ccsds::fileExists(configIt->second)) {
      return ccsds::Error{static_cast<ccsds::ErrorCode>(ARG_PARSE_ERROR),
                          "Configuration file does not exist: " + configIt->second};
    }

    ccsds::Config cfg;
    const auto loadResult = cfg.load(configIt->second);
    if (!loadResult) return loadResult.error();

    const auto templateResult = settings.templatePacket.loadFromConfig(cfg);
    if (!templateResult) return templateResult.error();

    settings.mode = settings.templatePacket.getPacketErrorControlMode();
    settings.hasTemplate = true;

    if (cfg.isKey("sync_pattern_enable")) {
      const auto syncEnableResult = cfg.get<bool>("sync_pattern_enable");
      if (!syncEnableResult) return syncEnableResult.error();
      settings.syncPatternEnable = syncEnableResult.value();
    }

    if (settings.syncPatternEnable && cfg.isKey("sync_pattern")) {
      const auto syncResult = cfg.get<int>("sync_pattern");
      if (!syncResult) return syncResult.error();
      settings.syncPattern = static_cast<std::uint32_t>(syncResult.value());
    }
  }

  const auto modeIt = args.find("packet-error-control");
  if (modeIt != args.end()) {
    const auto modeResult = parsePacketErrorControlMode(modeIt->second);
    if (!modeResult) return modeResult.error();
    settings.mode = modeResult.value();
    if (settings.hasTemplate) {
      const auto applied = applyPacketErrorControlMode(settings.templatePacket, settings.mode);
      if (!applied) return applied.error();
    }
  }

  return settings;
}

void emitValidationReport(const ccsds::ValidationReport &validation,
                          const bool verbose,
                          std::ostringstream &report) {
  for (const auto &check : validation) {
    if (!verbose && check.passed) continue;
    std::ostringstream line;
    line << "  [REPORT] " << std::left << std::setw(37)
         << ccsds::validationCodeName(check.code)
         << " : " << (check.passed ? "PASSED" : "FAILED") << '\n';
    report << line.str();
    std::cout << line.str();
  }
}

void emitParseFailure(const ccsds::Error &error, const bool pusHeaderExpected) {
  const auto &message = error.message();
  if (pusHeaderExpected) {
    std::cout << "  [REPORT] PUS secondary header                  : FAILED\n";
  } else if (message.find("packet version") != std::string::npos) {
    std::cout << "  [REPORT] CCSDS version                         : FAILED\n";
  } else {
    std::cout << "  [REPORT] Packet parse                          : FAILED\n";
  }
  std::cerr << "  " << message << std::endl;
}

ccsds::Packet parserPacket(const ValidatorSettings &settings) {
  ccsds::Packet packet = settings.hasTemplate ? settings.templatePacket : ccsds::Packet{};
  packet.setPacketErrorControlMode(settings.mode);
  return packet;
}

bool expectsPusHeader(const ccsds::Packet &packet) {
  const auto header = packet.getSecondaryHeader();
  return header && header->isPusHeader();
}

} // namespace

int main(const int argc, char *argv[]) {
  const std::string appName = "ccsds_validator";
  std::unordered_map<std::string, std::string> allowed{
    {"h", "help"}, {"v", "verbose"}, {"i", "input"},
    {"c", "config"}, {"p", "print-packets"},
    {"e", "packet-error-control"}
  };
  const std::set<std::string> booleanArgs{"verbose", "help", "print-packets"};
  std::unordered_map<std::string, std::string> args{
    {"verbose", "false"}, {"help", "false"}, {"print-packets", "false"}
  };

  const auto start = std::chrono::high_resolution_clock::now();
  const auto argumentResult = parseArguments(argc, argv, allowed, args, booleanArgs);
  if (!argumentResult) return printError(argumentResult.error());

  if (args["help"] == "true") {
    printHelp();
    return 0;
  }

  const auto inputIt = args.find("input");
  if (inputIt == args.end() || inputIt->second.empty()) {
    printHelp();
    return printError(ccsds::Error{static_cast<ccsds::ErrorCode>(ARG_PARSE_ERROR),
                                   "Input file must be specified"});
  }
  if (!ccsds::fileExists(inputIt->second)) {
    return printError(ccsds::Error{static_cast<ccsds::ErrorCode>(ARG_PARSE_ERROR),
                                   "Input file does not exist: " + inputIt->second});
  }

  const auto settingsResult = loadValidatorSettings(args);
  if (!settingsResult) return printError(settingsResult.error());
  const ValidatorSettings settings = settingsResult.value();

  const auto inputResult = ccsds::readBinaryFile(inputIt->second);
  if (!inputResult) return printError(inputResult.error());
  const std::vector<std::uint8_t> inputBytes = inputResult.value();

  const auto layoutResult = inspectPacketStream(inputBytes,
                                                settings.syncPatternEnable,
                                                settings.syncPattern,
                                                true);
  if (!layoutResult) {
    std::cout << "  [REPORT] Packet Data Length                   : FAILED\n";
    std::cerr << "  " << layoutResult.error().message() << std::endl;
    customConsole(appName, "Packets validation [FAILED]");
    return PACKET_VALIDATION_FAILED;
  }
  const PacketStreamLayout layout = layoutResult.value();

  if (layout.packets.empty()) {
    std::cout << "  [REPORT] Packet Data Length                   : FAILED\n";
    std::cerr << "  Input does not contain a complete CCSDS packet" << std::endl;
    customConsole(appName, "Packets validation [FAILED]");
    return PACKET_VALIDATION_FAILED;
  }

  const bool verbose = args["verbose"] == "true";
  const bool printPacketsEnabled = args["print-packets"] == "true";
  bool overallResult{true};
  std::vector<std::size_t> failedPackets;

  ccsds::Validator validator;
  validator.configure(true, true, settings.hasTemplate);
  if (settings.hasTemplate) validator.setTemplatePacket(settings.templatePacket);

  for (std::size_t index = 0U; index < layout.packets.size(); ++index) {
    const auto &slice = layout.packets[index];
    const std::vector<std::uint8_t> packetBytes(
      inputBytes.begin() + static_cast<std::ptrdiff_t>(slice.offset),
      inputBytes.begin() + static_cast<std::ptrdiff_t>(slice.offset + slice.size));

    std::ostringstream packetReport;
    packetReport << "[ CCSDS VALIDATOR ] Packet " << index + 1U << '\n';
    if (verbose) std::cout << packetReport.str();

    ccsds::Packet packet = parserPacket(settings);
    const bool pusExpected = expectsPusHeader(packet);
    const auto parsed = packet.deserializeBounded(packetBytes);
    if (!parsed) {
      overallResult = false;
      failedPackets.push_back(index + 1U);
      emitParseFailure(parsed.error(), pusExpected);
      continue;
    }

    const auto validation = validator.validate(packet);
    emitValidationReport(validation, verbose, packetReport);

    if (!validation.valid()) {
      overallResult = false;
      failedPackets.push_back(index + 1U);
    }

    if (printPacketsEnabled) ccsds::printPacket(packet);
  }

  const std::size_t trailingBytes = inputBytes.size() - layout.consumedBytes;
  if (trailingBytes != 0U) {
    overallResult = false;
    std::cout << "  [REPORT] Packet Data Length                   : FAILED\n";
    std::cerr << "  " << trailingBytes
              << " trailing byte(s) do not form a complete packet" << std::endl;
  }

  if (!failedPackets.empty()) {
    std::cerr << "[ CCSDS VALIDATOR ] Failed packet(s):";
    for (const auto packetIndex : failedPackets) std::cerr << ' ' << packetIndex;
    std::cerr << std::endl;
  }

  customConsole(appName,
                std::string("packet error control: ")
                  + packetErrorControlModeName(settings.mode));
  customConsole(appName,
                std::string("Packets validation [")
                  + (overallResult ? "PASSED" : "FAILED") + "]");

  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::high_resolution_clock::now() - start);
  customConsole(appName, "execution time: " + std::to_string(duration.count()) + " [us]");

  if (!overallResult) {
    customConsole(appName, "[ Exit code 18 ]");
    return PACKET_VALIDATION_FAILED;
  }

  customConsole(appName, "[ Exit code 0 ]");
  return 0;
}
