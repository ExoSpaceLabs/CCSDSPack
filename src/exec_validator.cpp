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
    << "  -v, --verbose                          Print every check for every packet\n"
    << "  -p, --print-packets                    Print packets that pass parse-time checks\n"
    << "  -h, --help                             Show this help message\n\n"
    << "Validation reports Packet Data Length, CRC16, CCSDS version, APID, packet\n"
    << "type, secondary-header flag, sequence flags, and sequence-count continuity\n"
    << "as separate checks. A config is required for APID/type/header-flag comparison.\n";
}

int printError(const CCSDS::Error &error) {
  std::cerr << "[ Error " << static_cast<unsigned>(error.code()) << " ]: "
            << error.message() << std::endl;
  return static_cast<int>(error.code());
}

struct ValidatorSettings {
  CCSDS::PacketErrorControlMode mode{CCSDS::PacketErrorControlMode::CRC16};
  bool syncPatternEnable{};
  std::uint32_t syncPattern{0x1ACFFC1DU};
  bool hasTemplate{};
  CCSDS::Packet templatePacket{};
};

CCSDS::Result<ValidatorSettings>
loadValidatorSettings(const std::unordered_map<std::string, std::string> &args) {
  ValidatorSettings settings;

  const auto configIt = args.find("config");
  if (configIt != args.end()) {
    if (!fileExists(configIt->second)) {
      return CCSDS::Error{static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                          "Config file does not exist: " + configIt->second};
    }

    Config cfg;
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
      settings.templatePacket.setPacketErrorControlMode(settings.mode);
    }
  }

  return settings;
}

struct SequenceState {
  bool initialized{};
  bool segmentOpen{};
  std::uint16_t expectedCount{};
};

struct PacketChecks {
  bool length{true};
  bool crc{true};
  bool version{true};
  bool apid{true};
  bool packetType{true};
  bool secondaryHeaderFlag{true};
  bool sequenceFlags{true};
  bool sequenceCount{true};
  bool crcChecked{};
  bool apidChecked{};
  bool identifierChecked{};
};

const char *statusText(const bool passed, const bool checked) {
  if (!checked) return "NOT CHECKED";
  return passed ? "PASSED" : "FAILED";
}

void emitCheck(const std::string &name,
               const bool passed,
               const bool checked,
               const bool verbose,
               std::ostringstream &report) {
  std::ostringstream line;
  line << "  [REPORT] " << std::left << std::setw(31) << name
       << " : " << statusText(passed, checked) << '\n';
  report << line.str();
  if (verbose || (checked && !passed)) std::cout << line.str();
}

void updateSequenceState(const CCSDS::Header &header,
                         SequenceState &state,
                         PacketChecks &checks) {
  const auto flags = static_cast<CCSDS::ESequenceFlag>(header.getSequenceFlags());

  switch (flags) {
    case CCSDS::UNSEGMENTED:
    case CCSDS::FIRST_SEGMENT:
      checks.sequenceFlags = !state.segmentOpen;
      break;
    case CCSDS::CONTINUING_SEGMENT:
    case CCSDS::LAST_SEGMENT:
      checks.sequenceFlags = state.segmentOpen;
      break;
    default:
      checks.sequenceFlags = false;
      break;
  }

  checks.sequenceCount = !state.initialized
                         || header.getSequenceCount() == state.expectedCount;

  if (!checks.sequenceFlags || !checks.sequenceCount) return;

  state.initialized = true;
  state.expectedCount = static_cast<std::uint16_t>(
    (header.getSequenceCount() + 1U) & 0x3FFFU);
  state.segmentOpen = flags == CCSDS::FIRST_SEGMENT
                      || flags == CCSDS::CONTINUING_SEGMENT;
}

PacketChecks validatePacketBytes(const std::vector<std::uint8_t> &packetBytes,
                                 const CCSDS::Header &header,
                                 const ValidatorSettings &settings,
                                 SequenceState &sequenceState) {
  PacketChecks checks;

  const std::size_t declaredSize =
    6U + static_cast<std::size_t>(header.getDataLength()) + 1U;
  checks.length = packetBytes.size() == declaredSize;
  checks.version = header.getVersionNumber() == 0U;

  checks.crcChecked = settings.mode == CCSDS::PacketErrorControlMode::CRC16;
  if (checks.crcChecked) {
    checks.crc = packetBytes.size() >= 8U;
    if (checks.crc) {
      const std::uint16_t received = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(packetBytes[packetBytes.size() - 2U]) << 8U)
        | packetBytes.back());
      const std::vector<std::uint8_t> crcInput(packetBytes.begin(),
                                               packetBytes.end() - 2);
      checks.crc = ::crc16(crcInput) == received;
    }
  }

  if (settings.hasTemplate) {
    const auto &templateHeader = settings.templatePacket.getPrimaryHeader();
    checks.apidChecked = true;
    checks.identifierChecked = true;
    checks.apid = header.getAPID() == templateHeader.getAPID();
    checks.packetType = header.getType() == templateHeader.getType();
    checks.secondaryHeaderFlag =
      header.getDataFieldHeaderFlag() == templateHeader.getDataFieldHeaderFlag();
  }

  updateSequenceState(header, sequenceState, checks);
  return checks;
}

bool allChecksPass(const PacketChecks &checks) {
  return checks.length
         && (!checks.crcChecked || checks.crc)
         && checks.version
         && (!checks.apidChecked || checks.apid)
         && (!checks.identifierChecked || checks.packetType)
         && (!checks.identifierChecked || checks.secondaryHeaderFlag)
         && checks.sequenceFlags
         && checks.sequenceCount;
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
    return printError(CCSDS::Error{static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                                   "Input file must be specified"});
  }
  if (!fileExists(inputIt->second)) {
    return printError(CCSDS::Error{static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR),
                                   "Input file does not exist: " + inputIt->second});
  }

  const auto settingsResult = loadValidatorSettings(args);
  if (!settingsResult) return printError(settingsResult.error());
  const ValidatorSettings settings = settingsResult.value();

  const auto inputResult = readBinaryFile(inputIt->second);
  if (!inputResult) return printError(inputResult.error());
  const std::vector<std::uint8_t> inputBytes = inputResult.value();

  const auto layoutResult = inspectPacketStream(inputBytes,
                                                settings.syncPatternEnable,
                                                settings.syncPattern,
                                                true);
  if (!layoutResult) {
    std::cout << "  [REPORT] Packet Data Length              : FAILED\n";
    std::cerr << "  " << layoutResult.error().message() << std::endl;
    customConsole(appName, "Packets validation [FAILED]");
    return PACKET_VALIDATION_FAILED;
  }
  const PacketStreamLayout layout = layoutResult.value();

  if (layout.packets.empty()) {
    std::cout << "  [REPORT] Packet Data Length              : FAILED\n";
    std::cerr << "  Input does not contain a complete CCSDS packet" << std::endl;
    customConsole(appName, "Packets validation [FAILED]");
    return PACKET_VALIDATION_FAILED;
  }

  const bool verbose = args["verbose"] == "true";
  const bool printPacketsEnabled = args["print-packets"] == "true";
  SequenceState sequenceState;
  bool overallResult{true};
  std::vector<std::size_t> failedPackets;

  for (std::size_t index = 0; index < layout.packets.size(); ++index) {
    const auto &slice = layout.packets[index];
    const std::vector<std::uint8_t> packetBytes(
      inputBytes.begin() + static_cast<std::ptrdiff_t>(slice.offset),
      inputBytes.begin() + static_cast<std::ptrdiff_t>(slice.offset + slice.size));
    const std::vector<std::uint8_t> headerBytes(packetBytes.begin(),
                                                packetBytes.begin() + 6);

    CCSDS::Header header;
    const auto headerResult = header.deserialize(headerBytes);
    if (!headerResult) {
      std::cout << "[ CCSDS VALIDATOR ] Packet " << index + 1U << '\n';
      std::cout << "  [REPORT] CCSDS primary header            : FAILED\n";
      failedPackets.push_back(index + 1U);
      overallResult = false;
      continue;
    }

    const PacketChecks checks = validatePacketBytes(packetBytes,
                                                    header,
                                                    settings,
                                                    sequenceState);
    std::ostringstream packetReport;
    packetReport << "[ CCSDS VALIDATOR ] Packet " << index + 1U << '\n';
    if (verbose) std::cout << packetReport.str();

    emitCheck("Packet Data Length", checks.length, true, verbose, packetReport);
    emitCheck("CRC16", checks.crc, checks.crcChecked, verbose, packetReport);
    emitCheck("CCSDS version", checks.version, true, verbose, packetReport);
    emitCheck("APID", checks.apid, checks.apidChecked, verbose, packetReport);
    emitCheck("Packet type", checks.packetType,
              checks.identifierChecked, verbose, packetReport);
    emitCheck("Secondary-header flag", checks.secondaryHeaderFlag,
              checks.identifierChecked, verbose, packetReport);
    emitCheck("Sequence flags", checks.sequenceFlags, true, verbose, packetReport);
    emitCheck("Sequence count", checks.sequenceCount, true, verbose, packetReport);

    const bool packetPassed = allChecksPass(checks);
    overallResult = overallResult && packetPassed;
    if (!packetPassed) failedPackets.push_back(index + 1U);

    if (printPacketsEnabled && checks.length
        && (!checks.crcChecked || checks.crc)
        && checks.version) {
      CCSDS::Packet packet;
      packet.setPacketErrorControlMode(settings.mode);
      const auto parseResult = packet.deserializeBounded(packetBytes);
      if (parseResult) printPacket(packet);
    }
  }

  const std::size_t trailingBytes = inputBytes.size() - layout.consumedBytes;
  if (trailingBytes != 0U) {
    overallResult = false;
    std::cout << "  [REPORT] Packet Data Length              : FAILED\n";
    std::cerr << "  " << trailingBytes
              << " trailing byte(s) do not form a complete packet" << std::endl;
  }

  if (!failedPackets.empty()) {
    std::cerr << "[ CCSDS VALIDATOR ] Failed packet(s):";
    for (const auto packet : failedPackets) std::cerr << ' ' << packet;
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
