
/**
 * This is the source file that holds the execution logic of ccsds_encoder binary file.
 */

#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <locale>
#include <sstream>
#include "CCSDSManager.h"
#include "CCSDSHeader.h"
#include "CCSDSResult.h"
#include "CCSDSUtils.h"

enum ErrorCodeExec : uint8_t {
  ARG_PARSE_ERROR = 14,                           ///< Error Parsing argument
  OTHER = 15
};
CCSDS::ResultBool parseArguments(int argc, char *argv[],
                                 std::unordered_map<std::string, std::string> &allowedMap,
                                 std::unordered_map<std::string, std::string> &outArgs);

bool fileExists(const std::string &fileName);
void customConsole(const std::string& appName, const std::string& message, const std::string& logLevel = "INFO");
void printHelp() {
  std::cout << "Usage: ccsds_encoder [OPTIONS] - input and output file is mandatory." << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << " -h or --help              : Show this help and exit" << std::endl;
  std::cout << " -v or --verbose           : Show generated packets information" << std::endl;
  std::cout << " -i or --input <filename>  : input file to be encoded" << std::endl;;
  std::cout << " -o or --output <filename> : Generated output file" << std::endl;;
  std::cout << " -c or --config <filename> : Configuration file" << std::endl;
}

int main(const int argc, char* argv[]) {
  std::string appName = "ccsds_encoder";

  std::unordered_map<std::string, std::string> allowed;
  allowed.insert({"h", "help"});
  allowed.insert({"v", "verbose"});
  allowed.insert({"i", "input"});
  allowed.insert({"o", "output"});
  allowed.insert({"c", "config"});

  std::unordered_map<std::string, std::string> args;
  args.insert({"verbose", "false"});
  args.insert({"help", "false"});

  if (const auto exp = parseArguments(argc, argv, allowed, args); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }

  // std::cout << "Parsed args:\n";
  // for (const auto& [k, v] : args) {
  //   std::cout << "  " << k << ": " << v << '\n';
  // }

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

  std::vector<uint8_t> inputBytes;

  customConsole(appName,"reading data from " + input);
  if (const auto exp = readBinaryFile(input); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }else {
    inputBytes = exp.value();
  }

  // prepare template packet
  //todo implement configuration file reading.
  customConsole(appName,"creating CCSDS template packet");
  CCSDS::Header header;
  header.setVersionNumber(1);
  header.setType(1);
  header.setAPID(126);
  header.setDataFieldHeaderFlag(0);
  header.setSequenceFlags(CCSDS::ESequenceFlag::FIRST_SEGMENT);
  header.setSequenceCount(1);

  CCSDS::Packet templatePacket;
  templatePacket.setPrimaryHeader(header);

  CCSDS::Manager manager;
  if (const auto exp = manager.setPacketTemplate(templatePacket); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }
  manager.setDatFieldSize(256);
  manager.setSyncPatternEnable(false);
  customConsole(appName, "generating CCSDS packets using input data");
  if (const auto exp = manager.setApplicationData(inputBytes); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }
  if (verbose) printPackets(manager);

  customConsole(appName,"serializing CCSDS packets");
  auto packets = manager.getPacketsBuffer();
  customConsole(appName,"writing data to " + output);
  if (const auto exp = writeBinaryFile(packets, output); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }
  customConsole(appName,"done [Exit code: 0]");
  return 0;
}

CCSDS::ResultBool parseArguments(const int argc, char *argv[],
                                 std::unordered_map<std::string, std::string> &allowedMap,
                                 std::unordered_map<std::string, std::string> &outArgs)
{
  const std::set<std::string> booleanArgs{"verbose", "help"};
  std::set<std::string> allowedKeys;
  std::set<std::string> allowedShortKeys;
  for (const auto& [k, v] : allowedMap) {
    allowedShortKeys.insert(k);
    allowedKeys.insert(v);
  }
  for (int i = 1; i < argc; ++i) {
    std::string current = argv[i];

    // Check if this is a key
    if (current.rfind("--", 0) == 0 || current.rfind('-', 0) == 0) {
      std::string key;
      if (current.rfind("--", 0) == 0){
        key = current.substr(2);
      }else {
        std::string sKey = current.substr(1);
        RET_IF_ERR_MSG(allowedShortKeys.find(sKey) == allowedShortKeys.end(), static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR), "Unknown argument: -" + sKey);
        key = allowedMap.at(sKey);
      }
      RET_IF_ERR_MSG(allowedKeys.find(key) == allowedKeys.end(), static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR), "Unknown argument: --" + key);

      if (booleanArgs.find(key) != booleanArgs.end()) {
        outArgs[key] = "true";
      }else {
        RET_IF_ERR_MSG(i + 1 >= argc, static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR), "Missing value for argument: --" + key);
        const std::string value = argv[++i];
        outArgs[key] = value;
      }
    }else {
      return CCSDS::Error{ static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR), "Unknown argument:" + std::string(argv[i]) };
    }
  }
  return true;
}

/**
 * @brief filesystem check fore file existence prepared for both windows and linux.
 *
 * @param fileName std::string
 * @return bool
 */
bool fileExists(const std::string &fileName) {
#ifdef _WIN32
  // Check if the fileName contains any non-ASCII characters
  bool isUnicode = std::any_of(fileName.begin(), fileName.end(), [](unsigned char c) {
      return c & 0x80;  // Checks for characters outside the ASCII range
  });

  if (isUnicode) {
    std::wstring wFileName(fileName.begin(), fileName.end());
    return std::filesystem::exists(std::filesystem::path(wFileName));
  } else {
    return std::filesystem::exists(std::filesystem::path(fileName));
  }
#else
   return std::filesystem::exists(fileName);
#endif
}

void customConsole(const std::string& appName, const std::string& message, const std::string& logLevel ) {
  // Get current timestamp with high precision
  const auto now = std::chrono::high_resolution_clock::now();
  const auto duration = now.time_since_epoch();
  const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);

  // Get the current system time for date and time portion
  const std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  const std::tm* tm = std::localtime(&currentTime);

  // Format timestamp (YYYY-MM-DD HH:MM:SS)
  std::ostringstream timestampStream;
  timestampStream << std::put_time(tm, "%Y-%m-%d %H:%M:%S");

  // Output log with timestamp, application name, log level, and microseconds
  std::cout << "[" << timestampStream.str() << "."
            << std::setw(6) << std::setfill('0') << microseconds.count() % 1000000 // last 6 digits (microseconds)
            << "] [" << appName << "] "
            << "[" << logLevel << "] : "
            << message << std::endl;
}