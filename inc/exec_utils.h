//
// Created by dev on 5/13/25.
//

#ifndef EXEC_UTILS_H
#define EXEC_UTILS_H
#include "CCSDSResult.h"

enum ErrorCodeExec : uint8_t {
  ARG_PARSE_ERROR = 14,                           ///< Error Parsing argument
  CONFIG_MISSING_PARAMETER = 15,
  INVALID_INPUT_DATA = 16,
  OTHER = 17
};

CCSDS::ResultBool parseArguments(int argc, char *argv[],
                                 std::unordered_map<std::string, std::string> &allowedMap,
                                 std::unordered_map<std::string, std::string> &outArgs);

void customConsole(const std::string& appName, const std::string& message, const std::string& logLevel = "INFO");
#endif //EXEC_UTILS_H
