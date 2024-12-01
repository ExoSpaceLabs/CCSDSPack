#ifndef CCSDSUTILS_H
#define CCSDSUTILS_H

#include <string>
#include <cstdint>
#include <vector>


// functions
std::string getBinaryString(uint32_t value, int bits);
std::string getBitsSpaces(int num);
#endif // CCSDSUTILS_H

