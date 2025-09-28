//
// Created by dev on 5/13/25.
//

#ifndef EXEC_UTILS_H
#define EXEC_UTILS_H
#include <set>
#include "CCSDSResult.h"
#include <unordered_map>

enum ErrorCodeExec : std::uint8_t {
  ARG_PARSE_ERROR = 14,                           ///< Error Parsing argument
  CONFIG_MISSING_PARAMETER = 15,
  INVALID_INPUT_DATA = 16,
  OTHER = 17,
  PACKET_VALIDATION_FAILED = 18
};

/**
 * @brief Parses the input arguments with defined values returns an unordered map with given keys
 * and input values. boolean Args do not require values associated with them.
 *
 * @param argc
 * @param argv
 * @param allowedMap
 * @param outArgs
 * @param booleanArgs
 * @return CCSDS::ResultBool
 */
CCSDS::ResultBool parseArguments(int argc, char *argv[],
                                 std::unordered_map<std::string, std::string> &allowedMap,
                                 std::unordered_map<std::string, std::string> &outArgs,
                                 const std::set<std::string> &booleanArgs);

/**
 * @brief Prints log to console adding various information about.
 *
 * @param appName
 * @param message
 * @param logLevel
 */
void customConsole(const std::string& appName, const std::string& message, const std::string& logLevel = "INFO");
#endif //EXEC_UTILS_H
