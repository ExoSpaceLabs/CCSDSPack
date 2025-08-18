
/**
 * This is the source file that holds the execution logic of ccsds_encoder binary file.
 */

#include "exec_utils.h"
#include <locale>
#include <iomanip>
#include <chrono>
#include <set>

CCSDS::ResultBool parseArguments(const std::int32_t argc, char *argv[],
                                 std::unordered_map<std::string, std::string> &allowedMap,
                                 std::unordered_map<std::string, std::string> &outArgs, const std::set<std::string> &booleanArgs)
{
  std::set<std::string> allowedKeys;
  std::set<std::string> allowedShortKeys;
  for (const auto& [k, v] : allowedMap) {
    allowedShortKeys.insert(k);
    allowedKeys.insert(v);
  }
  for ( std::int32_t i = 1; i < argc; ++i) {
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