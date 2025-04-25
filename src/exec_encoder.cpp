
/**
 * This is the source file that holds the execution logic of ccsds_encoder binary file.
 */

#include <unordered_map>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <filesystem>

#include "CCSDSManager.h"
#include "CCSDSHeader.h"
#include "CCSDSResult.h"
#include "CCSDSUtils.h"

enum ErrorCodeExec : uint8_t {
  ARG_PARSE_ERROR = 14,                           ///< Error Parsing argument
  OTHER = 15
};
CCSDS::ResultBool parseArguments(int argc, char *argv[],
                                 const std::set<std::string> &allowedKeys,
                                 std::unordered_map<std::string, std::string> &outArgs);

int main(const int argc, char* argv[]) {
  const std::set<std::string> allowed = {"input", "mode", "output"};
  std::unordered_map<std::string, std::string> args;

  if (const auto exp = parseArguments(argc, argv, allowed, args); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }

  std::cout << "Parsed args:\n";
  for (const auto& [k, v] : args) {
    std::cout << "  " << k << ": " << v << '\n';
  }

  if (args.find("input") == args.end()) {
    std::cerr << "[ Error " << ARG_PARSE_ERROR << " ]: " << "Input file must be specified" << std::endl;
    return ARG_PARSE_ERROR;
  }
  if (!std::filesystem::exists(args["input"])) {
    std::cerr << "[ Error " << ARG_PARSE_ERROR << " ]: " << "Input \"" << args["input"] << "\" does not exist" << std::endl;
    return ARG_PARSE_ERROR;
  }
  const std::string input = args["input"];
  std::vector<uint8_t> inputBytes;
  if (const auto exp = readBinaryFile(input); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }else {
    inputBytes = exp.value();
  }

  // prepare template packet
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
  if (const auto exp = manager.setApplicationData(inputBytes); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }
  auto packets = manager.getPacketsBuffer();
  printPackets(manager);

  std::string output{args["output"]};

  if (const auto exp = writeBinaryFile(packets, output); !exp.has_value()) {
    std::cerr << "[ Error " << exp.error().code() << " ]: "<<  exp.error().message() << std::endl ;
    return exp.error().code();
  }

  return 0;
}


CCSDS::ResultBool parseArguments(const int argc, char *argv[],
                                 const std::set<std::string> &allowedKeys,
                                 std::unordered_map<std::string, std::string> &outArgs)
{
  bool inputFileSet{false};

  for (int i = 1; i < argc; ++i) {
    std::string current = argv[i];

    // Check if this is a key
    if (current.rfind("--", 0) == 0) {
      std::string key = current.substr(2);

      RET_IF_ERR_MSG(allowedKeys.find(key) == allowedKeys.end(), static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR), "Unknown argument: --" + key);
      RET_IF_ERR_MSG(i + 1 >= argc, static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR), "Missing value for argument: --" + key);

      const std::string value = argv[++i];
      outArgs[key] = value;
    } else if (!inputFileSet){
      const std::string value = argv[i];
      outArgs["input"] = value;
      inputFileSet = true;
    }else {
      return CCSDS::Error{ static_cast<CCSDS::ErrorCode>(ARG_PARSE_ERROR), "Unknown argument:" + std::string(argv[i]) };
    }
  }
  return true;
}

