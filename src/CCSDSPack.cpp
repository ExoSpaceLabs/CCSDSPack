

#include "CCSDSPack.h"
#include <iostream>


// ToDo
// these functions are utilities. not related to the project top me moved
std::string getBinaryString(uint32_t value, int bits) {
    std::string binaryString;
        // Calculate the minimum number of bits required to represent in groups of 4
    int paddedBits = ((bits + 3) / 4) * 4;  // Round up to the nearest multiple of 4

    for (int i = paddedBits - 1; i >= 0; --i) {
        binaryString += ((value >> i) & 1) ? '1' : '0';

        // Add a space after every 4 bits, except at the end
        if (i % 4 == 0 && i != 0) {
            binaryString += ' ';
        }
    }
    return binaryString;
}

std::string getBitsSpaces(int num){
    std::string spaces;
    
    for (int i = num - 1; i >= 0; --i) {
        spaces += ' ';
    }

    return spaces;
}


void CCSDSHeader::printHeader(){

    std::cout << std::endl;
    std::cout << " [CCSDS HEADER] Test result:" << std::endl;                                                               
    std::cout << " [CCSDS HEADER] Full Primary Header [Hex]    : [ " << getBitsSpaces(17-12) << "0x" << std::hex << getFullHeader() << " ]" << std::endl;
    std::cout << std::endl;
    std::cout << " [CCSDS HEADER] Info: Version Number         : [ " << getBitsSpaces(19- 4) << getBinaryString(       getVersionNumber(),  3 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Type                   : [ " << getBitsSpaces(19- 4) << getBinaryString(                getType(),  1 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Data Field Header Flag : [ " << getBitsSpaces(19- 4) << getBinaryString( getDataFieldheaderFlag(),  1 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: APID                   : [ " << getBitsSpaces(17-12) << getBinaryString(                getAPID(), 11 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Sequence Flags         : [ " << getBitsSpaces(19- 4) << getBinaryString(       getSequenceFlags(),  2 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: Sequence Count         : [ " << getBitsSpaces(16-16) << getBinaryString(       getSequenceCount(), 14 ) << " ]" << std::endl;
    std::cout << " [CCSDS HEADER] Info: DataLength             : [ "                         << getBinaryString(          getDataLength(), 16 ) << " ]" << std::endl;
    std::cout << std::endl;
}



// constructor
CCSDSPack::CCSDSPack() {}

// methods
