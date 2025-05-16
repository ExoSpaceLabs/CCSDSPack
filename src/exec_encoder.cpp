
/**
 * This is the source file that holds the execution logic of ccsds_encoder binary file.
 */

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <sstream>
#include "CCSDSPack.h"
#include "exec_utils.h"


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
  "▐        ███████╗███╗   ██╗ ██████╗ ██████╗ ██████╗ ███████╗██████╗        ▌\n"
  "▐        ██╔════╝████╗  ██║██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔══██╗       ▌\n"
  "▐        █████╗  ██╔██╗ ██║██║     ██║   ██║██║  ██║█████╗  ██████╔╝       ▌\n"
  "▐        ██╔══╝  ██║╚██╗██║██║     ██║   ██║██║  ██║██╔══╝  ██╔══██╗       ▌\n"
  "▐        ███████╗██║ ╚████║╚██████╗╚██████╔╝██████╔╝███████╗██║  ██║       ▌\n"
  "▐        ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝╚═╝  ╚═╝       ▌\n"
  "▐▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▌\n"
  << std::endl;
  std::cout << "Usage: ccsds_encoder [OPTIONS] - take a file and packets it into ccsds packets and saves it to a binary file." << std::endl;
  std::cout << "Mandatory parameters:" << std::endl;
  std::cout << " -i or --input <filename>  : input file to be encoded" << std::endl;;
  std::cout << " -o or --output <filename> : Generated output file" << std::endl;;
  std::cout << " -c or --config <filename> : Configuration file" << std::endl;
  std::cout << std::endl;
  std::cout << "Optionals:" << std::endl;
  std::cout << " -h or --help              : Show this help and message" << std::endl;
  std::cout << " -v or --verbose           : Show generated packets information" << std::endl;
  std::cout << std::endl;
  std::cout << "Template override: the template CCSDS packet read from the config file" << std::endl;
  std::cout << "can be overwritten by using the following options. In the case not all" << std::endl;
  std::cout << "parameters are used, the remaining parameters will be read from the" << std::endl;
  std::cout << "configuration file." << std::endl;
  std::cout << " -tv <int>                 : Template CCSDS version number (3 bits)" << std::endl;
  std::cout << " -tt <bool>                : Template CCSDS Type" << std::endl;
  std::cout << " -ta <int>                 : Template CCSDS APID (11 bita)" << std::endl;
  std::cout << " -th <bool>                : Template CCSDS Secondary header presence" << std::endl;
  std::cout << " -ts <bool>                : Template CCSDS Segmented" << std::endl;
  std::cout << std::endl;
  std::cout << "For further information please visit: https://github.com/ExoSpaceLabs/CCSDSPack" << std::endl;
}

int main(const int argc, char* argv[]) {
  std::string appName = "ccsds_encoder";

  std::unordered_map<std::string, std::string> allowed;
  allowed.insert({"h", "help"});
  allowed.insert({"v", "verbose"});
  allowed.insert({"i", "input"});
  allowed.insert({"o", "output"});
  allowed.insert({"c", "config"});

  allowed.insert({"tv", "version_number"});
  allowed.insert({"tt", "type"});
  allowed.insert({"ta", "apid"});
  allowed.insert({"th", "secondary_header"});
  allowed.insert({"ts", "segmented"});

  std::unordered_map<std::string, std::string> args;
  args.insert({"verbose", "false"});
  args.insert({"help", "false"});

  const auto start = std::chrono::high_resolution_clock::now();
  if (const auto exp = parseArguments(argc, argv, allowed, args); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
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
  const std::string output{args["output"]};

  if (output.empty()) {
    std::cerr << "[ Error " << ARG_PARSE_ERROR << " ]: " << "Output file must be specified" << std::endl;
    printHelp();
    return ARG_PARSE_ERROR;
  }

  if (args.find("config") == args.end()) {
    std::cerr << "[ Error " << ARG_PARSE_ERROR << " ]: " << "Config file must be specified" << std::endl;
    printHelp();
    return ARG_PARSE_ERROR;
  }

  if (!fileExists(args["config"])) {
    std::cerr << "[ Error " << ARG_PARSE_ERROR << " ]: " << "Config \"" << args["config"] << "\" does not exist" << std::endl;
    return ARG_PARSE_ERROR;
  }
  const std::string configFile{args["config"]};

  // prepare template packet
  customConsole(appName,"reading CCSDS configuration file: " + configFile);
  Config cfg;
  {
    if (auto exp = cfg.load(configFile); !exp.has_value()) {
      std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
      return exp.error().code();
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
  if (args.find("version_number") == args.end()) {
    if (!cfg.isKey("ccsds_version_number")) {
      std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing int field: ccsds_version_number"
          << std::endl;
      return CONFIG_MISSING_PARAMETER;
    }
    ASSIGN_OR_PRINT(versionNumber, cfg.get<int>("ccsds_version_number"));
  }else {
    versionNumber = std::stoi(args["version_number"]);
  }

  if (args.find("type") == args.end()) {
    if (!cfg.isKey("ccsds_type")) {
      std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing bool field: ccsds_type" << std::endl;
      return CONFIG_MISSING_PARAMETER;
    }
    ASSIGN_OR_PRINT(type, cfg.get<bool>("ccsds_type"));
  }else {
    type = args["type"] == "true";
  }

  if (args.find("secondary_header") == args.end()) {
    if (!cfg.isKey("ccsds_data_field_header_flag")) {
      std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing bool field: ccsds_data_field_header_flag" << std::endl;
      return CONFIG_MISSING_PARAMETER;
    }
    ASSIGN_OR_PRINT(dataFieldHeaderFlag, cfg.get<bool>("ccsds_data_field_header_flag"));
  }else {
    dataFieldHeaderFlag = args["secondary_header"] == "true";
  }

  if (args.find("apid") == args.end()) {
    if (!cfg.isKey("ccsds_APID")) {
      std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing int field: ccsds_APID" << std::endl;
      return CONFIG_MISSING_PARAMETER;
    }
    ASSIGN_OR_PRINT(APID, cfg.get<int>("ccsds_APID"));
  }else {
    APID = std::stoi(args["apid"]);
  }

  if (args.find("segmented") == args.end()) {
    if (!cfg.isKey("ccsds_segmented")) {
      std::cerr << "[ Error " << CONFIG_MISSING_PARAMETER << " ]: " << "Config: Missing bool field: ccsds_segmented" << std::endl;
      return CONFIG_MISSING_PARAMETER;
    }
    ASSIGN_OR_PRINT(segmented, cfg.get<bool>("ccsds_segmented"));
  }else {
    segmented = args["segmented"] == "true";
  }

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
    if (auto exp = cfg.get<int>("sync_pattern"); exp.has_value()) {
      syncPattern = exp.value();
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

  CCSDS::Manager manager;
  if (const auto exp = manager.setPacketTemplate(templatePacket); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }
  manager.setDatFieldSize(dataFieldSize);
  manager.setSyncPatternEnable(syncPatternEnable);
  if (syncPatternEnable && cfg.isKey("sync_pattern")) {
    manager.setSyncPattern(syncPattern);
  }
  std::vector<uint8_t> inputBytes;

  customConsole(appName,"reading data from " + input);
  if (const auto exp = readBinaryFile(input); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }else {
    inputBytes = exp.value();
    if (!segmented && inputBytes.size() > dataFieldSize){
      std::cerr << "[ Error " << INVALID_INPUT_DATA << " ]: "<<  "Input data is too big for unsegmented packets, data "
      << inputBytes.size() << " must be less than defined data packet length of " << dataFieldSize << std::endl ;
      return INVALID_INPUT_DATA;
    }
  }
  customConsole(appName, "generating CCSDS packets using input data");
  if (const auto exp = manager.setApplicationData(inputBytes); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }
  if (verbose) customConsole(appName,"printing data to screen:");
  if (verbose) printPackets(manager);

  customConsole(appName,"serializing CCSDS packets");
  auto packets = manager.getPacketsBuffer();
  customConsole(appName,"writing data to " + output);
  if (const auto exp = writeBinaryFile(packets, output); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }

  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  customConsole(appName,"execution time: " + std::to_string(duration.count()) + " [us]");
  customConsole(appName,"[ Exit code 0 ]");
  return 0;
}
