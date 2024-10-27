#include <iostream>
#include <chrono>
#include "CCSDSPack.h"

int main() {

    {
    std::cout << "[ CCSDSPack ] Test using buffer as input." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    //==============================================================================
    // Start of the test
    
    uint64_t headerData = 0xFFFFFFFFFFFF;

    CCSDSPack ccsds;
    
    ccsds.setPrimaryHeader(headerData);    
    ccsds.printPrimaryHeader();
    
    // End of the Test
    //==============================================================================
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed = end - start;
    auto us = elapsed.count() * 1000000;
    
    std::cout << std::endl;
    //std::cout << "[ CCSDSPack ] Elapsed : " << elapsed << " [double]." << std::endl;
    std::cout << "[ CCSDSPack ] Elapsed time: " << us << " [us]." << std::endl;
    }
    
    //==============================================================================
    
    {
    std::cout << "[ CCSDSPack ] Test using the Struct as input." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    //==============================================================================
    // Start of the test
    
    //uint64_t headerData = 0xFFFFFFFFFFFF;
    PrimaryHeader headerData(1,1,1,1,1,1,1);

    CCSDSPack ccsds;
    
    ccsds.setPrimaryHeader(headerData);    
    ccsds.printPrimaryHeader();
    
    
    // End of the Test
    //==============================================================================
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> elapsed = end - start;
    auto us = elapsed.count() * 1000000;
    
    std::cout << std::endl;
    std::cout << "[ CCSDSPack ] Elapsed time: " << us << " [us]." << std::endl;
    }
    return 0;
}

