#include <iostream>
#include <chrono>
#include "CCSDSPack.h"

#include <cstring>

int main() {

    {
    std::cout << "[ CCSDSPack ] Test using buffer as input." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    //==============================================================================
    // Start of the test
    
    uint64_t headerData = 0xFFFFFFFFFFFF;
    CCSDS::Packet ccsds;
    ccsds.setPrimaryHeader(headerData);
    
    // End of the Test
    //==============================================================================
    auto end = std::chrono::high_resolution_clock::now();

    ccsds.printPrimaryHeader();
    
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
    CCSDS::PrimaryHeader headerData(1,
        1,
        1,
        1,
        1,
        1,
        1);
    CCSDS::Packet ccsds;
    ccsds.setPrimaryHeader(headerData);


    // End of the Test
    //==============================================================================
    auto end = std::chrono::high_resolution_clock::now();

    ccsds.printPrimaryHeader();

    std::chrono::duration<double> elapsed = end - start;
    auto us = elapsed.count() * 1000000;

    std::cout << std::endl;
    std::cout << "[ CCSDSPack ] Elapsed time: " << us << " [us]." << std::endl;
    }

    //==============================================================================

    {
        std::cout << "[ CCSDSPack ] Test Data field with crc" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        //==============================================================================
        // Start of the test
        CCSDS::Packet ccsds;
        ccsds.setDataField({1, 2, 3, 4, 5});



        // End of the Test
        //==============================================================================
        auto end = std::chrono::high_resolution_clock::now();

        ccsds.printDataField();

        std::chrono::duration<double> elapsed = end - start;
        auto us = elapsed.count() * 1000000;

        std::cout << std::endl;
        std::cout << "[ CCSDSPack ] Elapsed time: " << us << " [us]." << std::endl;
    }

    //==============================================================================

    {
        std::cout << "[ CCSDSPack ] Test Secondary Header and Data field with crc" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        //==============================================================================
        // Start of the test
        CCSDS::Packet ccsds;

        ccsds.setSecondaryHeader({0x1,0x2,0x3});
        ccsds.setDataField({4, 5});



        // End of the Test
        //==============================================================================
        auto end = std::chrono::high_resolution_clock::now();

        ccsds.printDataField();

        std::chrono::duration<double> elapsed = end - start;
        auto us = elapsed.count() * 1000000;

        std::cout << std::endl;
        std::cout << "[ CCSDSPack ] Elapsed time: " << us << " [us]." << std::endl;
    }

    //==============================================================================

    {
        std::cout << "[ CCSDSPack ] Test Data field * with crc" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        //==============================================================================
        // Start of the test
        CCSDS::Packet ccsds;

        uint8_t data[] = {0x1,0x2,0x3,0x4,0x5};

        ccsds.setDataField( data,5);



        // End of the Test
        //==============================================================================
        auto end = std::chrono::high_resolution_clock::now();

        ccsds.printDataField();

        std::chrono::duration<double> elapsed = end - start;
        auto us = elapsed.count() * 1000000;

        std::cout << std::endl;
        std::cout << "[ CCSDSPack ] Elapsed time: " << us << " [us]." << std::endl;
    }

    //==============================================================================

    {
        std::cout << "[ CCSDSPack ] Test Secondary header field * with crc" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        //==============================================================================
        // Start of the test
        CCSDS::Packet ccsds;

        uint8_t secondaryHeader[] = {0x1,0x2};
        uint8_t data[] = {0x3,0x4,0x5};

        ccsds.setDataField( data,3);
        ccsds.setSecondaryHeader( secondaryHeader, 2);



        // End of the Test
        //==============================================================================
        auto end = std::chrono::high_resolution_clock::now();

        ccsds.printDataField();

        std::chrono::duration<double> elapsed = end - start;
        auto us = elapsed.count() * 1000000;

        std::cout << std::endl;
        std::cout << "[ CCSDSPack ] Elapsed time: " << us << " [us]." << std::endl;
    }
    return 0;
}

