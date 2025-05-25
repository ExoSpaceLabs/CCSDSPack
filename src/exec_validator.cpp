
/**
 * This is the source file that holds the execution logic of ccsds_encoder binary file.
 */

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include "CCSDSPack.h"
#include "exec_utils.h"
#include "../test/inc/TestManager.h"


void printHelp() {
  // ascii art generated on https://www.asciiart.eu/text-to-ascii-art
  // with ANSI SHADOW Font, with 80 and Block frame
  std::cout << std::endl <<
  "▐▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▌\n"
  "▐         ██████╗ ██████╗███████╗██████╗ ███████╗                          ▌\n"
  "▐        ██╔════╝██╔════╝██╔════╝██╔══██╗██╔════╝                          ▌\n"
  "▐        ██║     ██║     ███████╗██║  ██║███████╗                          ▌\n"
  "▐        ██║     ██║     ╚════██║██║  ██║╚════██║   █▀█░█▀█░█▀▀░█░█░       ▌\n"
  "▐        ╚██████╗╚██████╗███████║██████╔╝███████║   █▀▀░█▀█░█░░░█▀▄░       ▌\n"
  "▐         ╚═════╝ ╚═════╝╚══════╝╚═════╝ ╚══════╝   ▀░░░▀░▀░▀▀▀░▀░▀░       ▌\n"
  "▐  ██╗   ██╗ █████╗ ██╗     ██╗██████╗  █████╗ ████████╗ ██████╗ ██████╗   ▌\n"
  "▐  ██║   ██║██╔══██╗██║     ██║██╔══██╗██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗  ▌\n"
  "▐  ██║   ██║███████║██║     ██║██║  ██║███████║   ██║   ██║   ██║██████╔╝  ▌\n"
  "▐  ╚██╗ ██╔╝██╔══██║██║     ██║██║  ██║██╔══██║   ██║   ██║   ██║██╔══██╗  ▌\n"
  "▐   ╚████╔╝ ██║  ██║███████╗██║██████╔╝██║  ██║   ██║   ╚██████╔╝██║  ██║  ▌\n"
  "▐    ╚═══╝  ╚═╝  ╚═╝╚══════╝╚═╝╚═════╝ ╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝  ▌\n"
  "▐▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▌\n"
  << std::endl;
  std::cout << "Usage: ccsds_validator [OPTIONS] - take a binary file holding CCSDS packets and validates it." << std::endl;
  std::cout << "Mandatory parameters:" << std::endl;
  std::cout << " -i or --input <filename>  : input file to be encoded" << std::endl;;
  std::cout << std::endl;
  std::cout << "Optionals:" << std::endl;
  std::cout << " -c or --config <filename> : Configuration file" << std::endl;
  std::cout << " -h or --help              : Show this help and message" << std::endl;
  std::cout << " -v or --verbose           : Show generated packets information" << std::endl;
  std::cout << " -p or --print-packets     : Show generated packets information" << std::endl;
  std::cout << std::endl;
  std::cout << "Note : If config file is provided, the loaded packets are validated against the the template packet." << std::endl;
  std::cout << std::endl;
  std::cout << "For further information please visit: https://github.com/ExoSpaceLabs/CCSDSPack" << std::endl;
}

/**
 * @brief Returns a report of performed validation checks in a string format.
 *
 * - Packet Coherence:
 *     - index [0]: Data Field Length Header declared equals actual data field length
 *     - index [1]: CRC15 value declared equals calculated CRC16 of the data field
 *     - index [2]: Sequence Control flags and count coherence
 *     - index [3]: Sequence Control count coherence (incremental start from 1)
 * - Compare Against Template:
 *     - index [4]: Identification and Version in packet matches template
 *     - index [5]: Template Sequence Control match
 *
 * @return A vector of boolean results for each performed check.
 */
std::string printReport(const std::vector<bool>& report, bool& result, const bool validateAgainstTemplate) {

  std::string packetReport = "  [REPORT] Data Field Length and Primary Header       : ";
  packetReport += (report[0]) ? GREEN  + "Passed\n" +  RESET : RED +  "failed\n" + RESET;
  packetReport += "  [REPORT] CRC15 value coherence                      : " ;
  packetReport += (report[1]) ? GREEN  + "Passed\n" +  RESET : RED +  "failed\n" + RESET;
  packetReport += "  [REPORT] Sequence Control flags and count coherence : " ;
  packetReport += (report[2]) ? GREEN  + "Passed\n" +  RESET : RED +  "failed\n" + RESET;
  packetReport += "  [REPORT] Sequence Control count coherence           : " ;
  packetReport += (report[3]) ? GREEN  + "Passed\n" +  RESET : RED +  "failed\n" + RESET;

  result = report[0] && report[1] && report[2] && report[3];
  if (validateAgainstTemplate){
    packetReport += "  [REPORT] Identification and Version                 : " ;
    packetReport += (report[4]) ? GREEN  + "Passed\n" +  RESET : RED +  "failed\n" + RESET;
    packetReport += "  [REPORT] Template Sequence Control                  : " ;
    packetReport += (report[5]) ? GREEN  + "Passed\n" +  RESET : RED +  "failed\n" + RESET;
    result = result && report[4] && report[5];
  }
  return packetReport;
}

void stripAnsiCodes(std::string& input) {
  std::string output;
  output.reserve(input.size());  // Avoids reallocations

  for (size_t i = 0; i < input.size(); ) {
    if (input[i] == '\033' && i + 1 < input.size() && input[i + 1] == '[') {
      // Skip ANSI escape sequence
      i += 2;
      while (i < input.size() && (input[i] < '@' || input[i] > '~')) {
        ++i;
      }
      if (i < input.size()) ++i; // skip final 'm' or other final char
    } else {
      output += input[i++];
    }
  }

  input = std::move(output);
}

int main(const int argc, char* argv[]) {
  std::string appName = "ccsds_validator";

  std::unordered_map<std::string, std::string> allowed;
  allowed.insert({"h", "help"});
  allowed.insert({"v", "verbose"});
  allowed.insert({"i", "input"});
  allowed.insert({"c", "config"});
  allowed.insert({"p", "print-packets"});

  const std::set<std::string> booleanArgs{"verbose", "help", "print-packets"};

  std::unordered_map<std::string, std::string> args;
  args.insert({"verbose", "false"});
  args.insert({"print", "false"});
  args.insert({"print-packets", "false"});

  const auto start = std::chrono::high_resolution_clock::now();
  if (const auto res = parseArguments(argc, argv, allowed, args, booleanArgs); !res.has_value()) {
    std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
    return res.error().code();
  }
   // std::cout << "Parsed args:\n";
   //for (const auto& [k, v] : args) {
   //  std::cout << "  " << k << ": " << v << '\n';
   //}
  if (args["help"] == "true") {
    printHelp();
    return 0;
  }
  bool verbose{args["verbose"] == "true"};

  if (args.find("input") == args.end()) {
    std::cerr << "[ Error " << ARG_PARSE_ERROR << " ]: " << "Input file must be specified" << std::endl;
    printHelp();
    return ARG_PARSE_ERROR;
  }

  if (!fileExists(args["input"])) {
    std::cerr << "[ Error " << ARG_PARSE_ERROR << " ]: " << "Input \"" << args["input"] << "\" does not exist" << std::endl;
    return ARG_PARSE_ERROR;
  }
  const std::string input{args["input"]};

  CCSDS::Manager manager;
  CCSDS::Validator validator;
  std::vector<uint8_t> inputBytes;
  bool isConfigProvided{false};

  manager.setAutoValidateEnable(false);
  manager.setDatFieldSize(64*1023 ); // 1M Bytes * packet
  manager.setAutoUpdateEnable(false);
  customConsole(appName,"reading data from " + input);
  if (const auto res = readBinaryFile(input); !res.has_value()) {
    std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
    return res.error().code();
  }else {
    inputBytes = res.value();
  }

  customConsole(appName, "deserializing CCSDS packets from file");
  if (const auto res = manager.load(inputBytes); !res.has_value()) {
    std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
    return res.error().code();
  }

  // config argument specified
  if (args.find("config") != args.end()) {
    isConfigProvided = true;
    if (!fileExists(args["config"])) {
      std::cerr << "[ Error " << ARG_PARSE_ERROR << " ]: " << "Config \"" << args["config"] << "\" does not exist" << std::endl;
      return ARG_PARSE_ERROR;
    }
    const std::string configFile{args["config"]};

    // prepare template packet
    customConsole(appName,"reading CCSDS configuration file: " + configFile);
    Config cfg;

    {
      if (auto res = cfg.load(configFile); !res.has_value()) {
        std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
        return res.error().code();
      }
    }

  CCSDS::Header header;
  uint8_t versionNumber;
  uint8_t type;
  uint8_t APID;
  uint8_t dataFieldHeaderFlag;
  uint16_t sequenceCount;
  CCSDS::ESequenceFlag sequenceFlag;
  uint16_t dataFieldSize;
  bool segmented;
  bool syncPatternEnable;
  uint32_t syncPattern;

  if (!cfg.isKey("ccsds_version_number")) {
    std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing int field: ccsds_version_number"
        << std::endl;
    return CONFIG_MISSING_PARAMETER;
  }
  ASSIGN_OR_PRINT(versionNumber, cfg.get<int>("ccsds_version_number"));


  if (!cfg.isKey("ccsds_type")) {
    std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing bool field: ccsds_type" << std::endl;
    return CONFIG_MISSING_PARAMETER;
  }
  ASSIGN_OR_PRINT(type, cfg.get<bool>("ccsds_type"));


  if (!cfg.isKey("ccsds_data_field_header_flag")) {
    std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing bool field: ccsds_data_field_header_flag" << std::endl;
    return CONFIG_MISSING_PARAMETER;
  }
  ASSIGN_OR_PRINT(dataFieldHeaderFlag, cfg.get<bool>("ccsds_data_field_header_flag"));


  if (!cfg.isKey("ccsds_APID")) {
    std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing int field: ccsds_APID" << std::endl;
    return CONFIG_MISSING_PARAMETER;
  }
  ASSIGN_OR_PRINT(APID, cfg.get<int>("ccsds_APID"));


  if (!cfg.isKey("ccsds_segmented")) {
    std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing bool field: ccsds_segmented" << std::endl;
    return CONFIG_MISSING_PARAMETER;
  }
  ASSIGN_OR_PRINT(segmented, cfg.get<bool>("ccsds_segmented"));


  if (!cfg.isKey("data_field_size")) {
    std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing int field: data_field_size" << std::endl;
    return CONFIG_MISSING_PARAMETER;
  }

  if (!cfg.isKey("sync_pattern_enable")) {
    std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing int field: sync_pattern_enable" << std::endl;
    return CONFIG_MISSING_PARAMETER;
  }

  ASSIGN_OR_PRINT(dataFieldSize, cfg.get<int>("data_field_size"));
  ASSIGN_OR_PRINT(syncPatternEnable, cfg.get<bool>("sync_pattern_enable"));

  {  // optional definition of sync pattern
    if (auto res = cfg.get<int>("sync_pattern"); res.has_value()) {
      syncPattern = res.value();
    }
  }

  customConsole(appName,"creating CCSDS template packet");
  if (segmented) {
    sequenceCount = 1;
    sequenceFlag = CCSDS::ESequenceFlag::FIRST_SEGMENT;
  }else {
    sequenceCount = 0;
    sequenceFlag = CCSDS::ESequenceFlag::UNSEGMENTED;
  }

    header.setVersionNumber(versionNumber);
    header.setType(type);
    header.setAPID(APID);
    header.setDataFieldHeaderFlag(dataFieldHeaderFlag);
    header.setSequenceFlags(sequenceFlag);
    header.setSequenceCount(sequenceCount);

    CCSDS::Packet templatePacket;
    templatePacket.setPrimaryHeader(header);

    if (const auto res = manager.setPacketTemplate(templatePacket); !res.has_value()) {
      std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
      return res.error().code();
    }
    manager.setDatFieldSize(dataFieldSize);
    manager.setSyncPatternEnable(syncPatternEnable);
    if (syncPatternEnable && cfg.isKey("sync_pattern")) {
      manager.setSyncPattern(syncPattern);
    }

    if (!segmented && inputBytes.size() > dataFieldSize){
      std::cerr << "[ Error " << INVALID_INPUT_DATA << " ]: "<<  "Input data is too big for unsegmented packets, data "
      << inputBytes.size() << " must be less than defined data packet length of " << dataFieldSize << std::endl ;
      return INVALID_INPUT_DATA;
    }
    validator.setTemplatePacket(templatePacket);
    validator.configure(true,true,true);
  }// end if config provided

  validator.configure(true,true,false);

  if (verbose) customConsole(appName,"printing data to screen:");

  std::vector<std::vector<bool>> reports;
  std::string reportsStream;
  bool overallResult{true};
  int packetIndex{1};
  std::vector<int> failedPackets;
  const bool printPackets{args["print-packets"] == "true"};

  for (auto &packet : manager.getPackets()) {
    if (verbose) std::cout << "[ CCSDS VALIDATOR ] Printing Packet [ " << packetIndex << " ]: " << std::endl;
    reportsStream += "[ CCSDS VALIDATOR ] Packet report for id: [ " + std::to_string(packetIndex) + " ]\n";
    if (printPackets) printPacket(packet);
    validator.validate(packet);
    auto report = validator.getReport();
    reports.emplace_back(report);
    bool currentResult{false};
    std::string appendReport = printReport(report,currentResult, isConfigProvided);
    overallResult = overallResult && currentResult;
    reportsStream += appendReport;
    if (verbose) std::cout << appendReport << std::endl;
    if (verbose) std::cout << "[ CCSDS VALIDATOR ] Packet Result [ ";
    if (verbose && currentResult) std::cout << GREEN;
    else if (verbose) {
      std::cout << RED;
    }
    if (verbose && currentResult) std::cout <<  "PASSED" ;
    else if (verbose) {
      std::cout << "FAILED";
    }
    if (verbose) std::cout << RESET << " ]" << std::endl;
    if (!currentResult) failedPackets.push_back(packetIndex);
    packetIndex++;
  }

  if (!failedPackets.empty() && verbose) {
    std::string failedPacketsStream;
    failedPacketsStream = "[ CCSDS VALIDATOR ] Packets failed validation: [ ";
    for (const auto& packetID : failedPackets) {
      failedPacketsStream += std::to_string(packetID) + " ";
    }
    failedPacketsStream += "]\n" ;
    std::cout << failedPacketsStream << std::endl;
    reportsStream += failedPacketsStream;
  }
  std::string resultString = (overallResult) ? GREEN + "PASSED" + RESET : RED + "FAILED" + RESET;
  reportsStream += "[ CCSDS VALIDATOR ] Packets validation [" + resultString + "]";
  customConsole(appName,"Packets validation [" + resultString + "]");


  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  customConsole(appName,"execution time: " + std::to_string(duration.count()) + " [us]");
  if (!overallResult) {
    customConsole(appName,"[ Exit code 18 ]");
    return PACKET_VALIDATION_FAILED;
  }
  customConsole(appName,"[ Exit code 0 ]");
  return 0;
}
