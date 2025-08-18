
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
  std::cout << "Note : the template CCSDS packet is defined in the configuration file" << std::endl;
  std::cout << "       This should follow the guide lines provided in the link below." << std::endl;
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

  const std::set<std::string> booleanArgs{"verbose", "help"};

  std::unordered_map<std::string, std::string> args;
  args.insert({"verbose", "false"});
  args.insert({"help", "false"});

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
    if (auto res = cfg.load(configFile); !res.has_value()) {
      std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
      return res.error().code();
    }
  }

  CCSDS::Manager manager;

  if (cfg.isKey("data_field_size")) {
    std::uint16_t dataFieldSize;
    ASSIGN_OR_PRINT(dataFieldSize, cfg.get<int>("data_field_size"));
    manager.setDataFieldSize(dataFieldSize);
  }

  if (cfg.isKey("sync_pattern_enable")) {
    bool syncPatternEnable;
    ASSIGN_OR_PRINT(syncPatternEnable, cfg.get<bool>("sync_pattern_enable"));
    manager.setSyncPatternEnable(syncPatternEnable);
    if (syncPatternEnable && cfg.isKey("sync_pattern")) {
      std::uint32_t syncPattern;
      ASSIGN_OR_PRINT(syncPattern, cfg.get<int>("sync_pattern"));
      manager.setSyncPattern(syncPattern);
    }
  }

  customConsole(appName,"creating CCSDS template packet");

  if (const auto res = manager.loadTemplateConfig(cfg);!res.has_value()) {
    std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
    return res.error().code();
  }

  std::vector<std::uint8_t> inputBytes;
  customConsole(appName,"reading data from " + input);

  if (const auto res = readBinaryFile(input); !res.has_value()) {
    std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
    return res.error().code();
  }else {
    inputBytes = res.value();
    if (manager.getTemplate().getPrimaryHeader().getSequenceFlags() == CCSDS::UNSEGMENTED && inputBytes.size() > manager.getDataFieldSize()){
      std::cerr << "[ Error " << INVALID_INPUT_DATA << " ]: "<<  "Input data is too big for unsegmented packets, data "
      << inputBytes.size() << " must be less than defined data packet length of " << manager.getDataFieldSize() << std::endl ;
      return INVALID_INPUT_DATA;
    }
  }

  customConsole(appName, "generating CCSDS packets using input data");
  if (const auto res = manager.setApplicationData(inputBytes); !res.has_value()) {
    std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
    return res.error().code();
  }
  if (verbose) customConsole(appName,"printing data to screen:");
  if (verbose) printPackets(manager);

  customConsole(appName,"serializing CCSDS packets");
  auto packets = manager.getPacketsBuffer();
  customConsole(appName,"writing data to " + output);
  if (const auto res = writeBinaryFile(packets, output); !res.has_value()) {
    std::cerr << "[ Error " << res.error().code() << " ]: "<<  res.error().message() << std::endl ;
    return res.error().code();
  }

  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  customConsole(appName,"execution time: " + std::to_string(duration.count()) + " [us]");
  customConsole(appName,"[ Exit code 0 ]");
  return 0;
}
