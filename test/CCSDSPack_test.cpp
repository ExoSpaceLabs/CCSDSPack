#include <iostream>
#include "CCSDSPack.h"
#include "CCSDSUtils.h"

#include <cstring>

int main() {
    Tester tester{};

    tester.unitTest("Assign Primary header using an uint64_t as input.", []() {
        constexpr uint64_t headerData( 0xFFFFFFFFFFFF );
        CCSDS::Packet ccsds;
        ccsds.setPrimaryHeader(headerData);
        const auto ret = ccsds.getPrimaryHeader();
        return ret == 0xFFFFFFFFFFFF;
    });

    //==============================================================================

    tester.unitTest("Assign Primary header using PrimaryHeader struct as input.", []()
    {
        constexpr uint64_t expectedHeaderData( 0x380140010001 );
        const CCSDS::PrimaryHeader headerData(1,
            1,
            1,
            1,
            1,
            1,
            1);
        CCSDS::Packet ccsds;
        ccsds.setPrimaryHeader(headerData);
        const auto ret = ccsds.getPrimaryHeader();
        return  ret == expectedHeaderData;
    });

    //==============================================================================

    {
        CCSDS::Packet ccsds;

        tester.unitTest("Assign Data field using vector, wDataFieldHeader should be empty.",[&ccsds]() {
            ccsds.setApplicationData({1, 2, 3, 4, 5});
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.empty();
        } );

        tester.unitTest("Assign Data field using vector, ApplicationData should be of correct size.",[&ccsds] {
            const auto apd = ccsds.getApplicationData();
            return apd.size() == 5;
        });

        tester.unitTest("Assign Data field using vector, CRC16 should be correct.", [&ccsds]() {
            ccsds.calculateCRC16();
            constexpr uint16_t expectedCRC16( 0x9304 );
            const auto crc( ccsds.getCRC() );
            return crc == expectedCRC16;
        });
    }

    //==============================================================================

    {
        CCSDS::Packet ccsds;

        tester.unitTest("Assign Secondary Header and Data using vector, wDataFieldHeader should be of correct size.",[&ccsds] {
            ccsds.setDataFieldHeader({0x1,0x2,0x3});
            ccsds.setApplicationData({4, 5});
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.size() == 3;
        });

        tester.unitTest("Assign Secondary Header and Data using vector, ApplicationData, should be of correct size.", [&ccsds] {
            const auto apd = ccsds.getApplicationData();
            return apd.size() == 2;
        });


        tester.unitTest("Assign Secondary Header and Data using vector, CRC16 should be correct.",[&ccsds] {
            ccsds.calculateCRC16();
            constexpr uint16_t expectedCRC16(0x9304);
            const auto crc(ccsds.getCRC());
            return crc == expectedCRC16;
        });
    }

    //==============================================================================

    {
        CCSDS::Packet ccsds;

        tester.unitTest("Assign Data field using array*, wDataFieldHeader should be empty.",[&ccsds] {
            const uint8_t data[] = {0x1,0x2,0x3,0x4,0x5};
            ccsds.setApplicationData( data,5);
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.empty();
        });

        tester.unitTest("Assign Data field using array*, ApplicationData should be of correct size.",[&ccsds] {
            const auto apd = ccsds.getApplicationData();
            return apd.size() == 5;
        });

        tester.unitTest("Assign Data field using array*, CRC16 should be correct.",[&ccsds] {
            ccsds.calculateCRC16();
            constexpr uint16_t expectedCRC16(0x9304);
            const auto crc(ccsds.getCRC());
            return crc == expectedCRC16;
        });
    }

    //==============================================================================

    {
        CCSDS::Packet ccsds;

        tester.unitTest("Assign Secondary header and data field using array*, wDataFieldHeader should be of correct size.", [&ccsds] {
            constexpr uint8_t secondaryHeader[] = {0x1,0x2};
            constexpr uint8_t data[] = {0x3,0x4,0x5};
            ccsds.setApplicationData( data,3);
            ccsds.setDataFieldHeader( secondaryHeader, 2);
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.size() == 2;
        });

        tester.unitTest("Assign Secondary header and data field using array*, ApplicationData should be of correct size.",[&ccsds] {
            const auto apd = ccsds.getApplicationData();
            return apd.size() == 3;
        });

        tester.unitTest("Assign Secondary header and data field using array*, CRC16 should be correct.",[&ccsds] {
            ccsds.calculateCRC16();
            constexpr uint16_t expectedCRC16(0x9304);
            const auto crc(ccsds.getCRC());
            return crc == expectedCRC16;
        });
    }

    //==============================================================================

    return tester.Result();
}

