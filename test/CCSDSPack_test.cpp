#include <iostream>
#include "CCSDSPacket.h"
#include "CCSDSUtils.h"

void testGroupBasic(TestManager *tester, const std::string& description) {
    std::cout << "  testGroupBasic: " << description <<  std::endl;

    tester->unitTest("Assign Primary header using an uint64_t as input.", []() {
        constexpr uint64_t headerData( 0xFFFFFFFFFFFF );
        CCSDS::Packet ccsds;
        ccsds.setPrimaryHeader(headerData);
        // getPrimaryHeader updated dependent fields to correct values.
        const auto ret = ccsds.getPrimaryHeader64bit();
        return ret == 0xf7ffc0000000;
    });

    tester->unitTest("Assign Primary header using PrimaryHeader struct as input.", []()
    {
        constexpr uint64_t expectedHeaderData( 0x300140000000 );
        const CCSDS::PrimaryHeader headerData(1,
            1,
            1,
            1,
            1,
            1,
            1);
        CCSDS::Packet ccsds;
        ccsds.setPrimaryHeader(headerData);
        // getPrimaryHeader updated dependent fields to correct values.
        const auto ret = ccsds.getPrimaryHeader64bit();
        return  ret == expectedHeaderData;
    });

    tester->unitTest("Assign Primary header performing vector deserialization.", []()
    {
        constexpr uint64_t expectedHeaderData( 0xf7ffc0000000 );
        const std::vector<uint8_t> data( {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} );
        CCSDS::Packet ccsds;
        ccsds.setPrimaryHeader(data);
        const auto ret = ccsds.getPrimaryHeader64bit();
        return  ret == expectedHeaderData;
    });

    tester->unitTest("Get Header using vector serialization.", []()
    {
        constexpr uint64_t data( 0xf7ffc0000000 );
        const std::vector<uint8_t> expectedHeaderData( {0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00} );
        CCSDS::Packet ccsds;
        ccsds.setPrimaryHeader(data);
        const auto ret = ccsds.getPrimaryHeader();
        return std::equal(expectedHeaderData.begin(), expectedHeaderData.end(), ret.begin());
    });

    {
        CCSDS::Packet ccsds;

        tester->unitTest("Assign Data field using vector, DataFieldHeader shall be empty.",[&ccsds]() {
            ccsds.setApplicationData({1, 2, 3, 4, 5});
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.empty();
        } );

        tester->unitTest("Assign Data field using vector, ApplicationData shall be of correct size.",[&ccsds] {
            const auto apd = ccsds.getApplicationData();
            return apd.size() == 5;
        });

        tester->unitTest("Assign Data field using vector, CRC16 shall be correct.", [&ccsds]() {
            constexpr uint16_t expectedCRC16( 0x9304 );
            const auto crc( ccsds.getCRC() );
            return crc == expectedCRC16;
        });
    }

    {
        CCSDS::Packet ccsds;

        tester->unitTest("Assign Secondary Header and Data using vector, DataFieldHeader shall be of correct size.",[&ccsds] {
            ccsds.setDataFieldHeader({0x1,0x2,0x3});
            ccsds.setApplicationData({4, 5});
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.size() == 3;
        });

        tester->unitTest("Assign Secondary Header and Data using vector, ApplicationData, shall be of correct size.", [&ccsds] {
            const auto apd = ccsds.getApplicationData();
            return apd.size() == 2;
        });


        tester->unitTest("Assign Secondary Header and Data using vector, CRC16 shall be correct.",[&ccsds] {
            constexpr uint16_t expectedCRC16(0x9304);
            const auto crc(ccsds.getCRC());
            return crc == expectedCRC16;
        });
    }

    {
        CCSDS::Packet ccsds;

        tester->unitTest("Assign Data field using array*, DataFieldHeader shall be empty.",[&ccsds] {
            const uint8_t data[] = {0x1,0x2,0x3,0x4,0x5};
            ccsds.setApplicationData( data,5);
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.empty();
        });

        tester->unitTest("Assign Data field using array*, ApplicationData shall be of correct size.",[&ccsds] {
            const auto apd = ccsds.getApplicationData();
            return apd.size() == 5;
        });

        tester->unitTest("Assign Data field using array*, CRC16 shall be correct.",[&ccsds] {
            constexpr uint16_t expectedCRC16(0x9304);
            const auto crc(ccsds.getCRC());
            return crc == expectedCRC16;
        });
    }

    {
        CCSDS::Packet ccsds;

        tester->unitTest("Primary header vector shall be 0 valued.",[&ccsds] {
            // although the header is set to FFFF for data field size, its content is updated by data field size.

            const auto header = ccsds.getPrimaryHeader();
            bool res(true);
            for (const auto v : header) {
                res &= v == 0;
            }
            return res;
        });
        tester->unitTest("Assign Secondary header and data field using array*, DataFieldHeader shall be of correct size.", [&ccsds] {
            constexpr uint8_t secondaryHeader[] = {0x1,0x2};
            constexpr uint8_t data[] = {0x3,0x4,0x5};
            ccsds.setApplicationData( data,3);
            ccsds.setDataFieldHeader( secondaryHeader, 2);
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.size() == 2;
        });

        tester->unitTest("Assign Secondary header and data field using array*, ApplicationData shall be of correct size.",[&ccsds] {
            const auto apd = ccsds.getApplicationData();
            return apd.size() == 3;
        });

        tester->unitTest("Assign Secondary header and data field using array*, CRC16 shall be correct.",[&ccsds] {
            constexpr uint16_t expectedCRC16(0x9304);
            const auto crc(ccsds.getCRC());
            return crc == expectedCRC16;
        });

        tester->unitTest("Primary header vector getter values shall be correctly returned.",[&ccsds] {
            ccsds.setPrimaryHeader(0xffffffffffff);
            // although the header is set to FFFF for data field size, its content is Header getters.
            const std::vector<uint8_t> expectedHeader = {
                0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x05
            };
            const auto header = ccsds.getPrimaryHeader();

            return std::equal(expectedHeader.begin(), expectedHeader.end(), header.begin());
        });

        tester->unitTest("CRC16 vector getter values shall be correctly returned.",[&ccsds] {
            const auto crc = ccsds.getCRCVector();
            auto res(true);
            res &= crc[0] == 0x93;
            res &= crc[1] == 0x04;;
            return res;
        });

        tester->unitTest("Get full packet size. Header, Data field and CRC shall be correctly positioned.",[&ccsds] {
            const auto header = ccsds.getPrimaryHeader();
            const auto data = ccsds.getFullDataField();
            const size_t packetSize = 6 + 2 + data.size();
            const auto packet = ccsds.serialize();
            bool res(packet.size() == packetSize);
            res &= header[3] == packet[3];
            res &= packet[6] == data[0];
            res &= packet[10] == data[4];
            const auto crc = ccsds.getCRCVector();
            res &= crc[0] == packet[packetSize-2];
            res &= crc[1] == packet[packetSize-1];;
            return res;
        });
    }
    {
        tester->unitTest("PUS-A secondary header assignment. returned data field header size is of correct size of 6 bytes.",[] {
            CCSDS::Packet ccsds;
            //const uint8_t data[] = {0x1,0x2,0x3,0x4,0x5};
            const CCSDS::PusA pusAHeader(1,2,3,4,5);
            ccsds.setDataFieldHeader(pusAHeader);
            const auto dfh = ccsds.getDataFieldHeader();
            return dfh.size() == pusAHeader.getSize();
        });

        tester->unitTest("PUS-B secondary header assignment. returned data field header size is of correct size of 8 bytes.",[] {
             CCSDS::Packet ccsds;
             const CCSDS::PusB pusBHeader(1,2,3,4,5,6);
             ccsds.setDataFieldHeader(pusBHeader);
             const auto dfh = ccsds.getDataFieldHeader();
             return dfh.size() == pusBHeader.getSize();
        });
    }
    {
        CCSDS::Packet ccsds;

        tester->unitTest("PUS-C secondary header assignment. returned data field header size is of correct size of 8 bytes.",[&ccsds] {

             const CCSDS::PusC pusCHeader(1,2,3,4,5,6);
             ccsds.setDataFieldHeader(pusCHeader);
             const auto dfh = ccsds.getDataFieldHeader();
             return dfh.size() == pusCHeader.getSize();
        });

        tester->unitTest("Automatic data Length update in Primary header (PUS-C size).",[&ccsds] {

            //ccsds.setPrimaryHeader(0xffffffff);
            //const uint8_t data[] = {0x1,0x2,0x3,0x4,0x5};
            const auto primaryHeader = ccsds.getPrimaryHeader64bit();
            return (primaryHeader & 0xFFFF) == 0x8;
        });

        tester->unitTest("Automatic data Length update in Primary header and Secondary header after data inclusion.",[&ccsds] {
            constexpr uint8_t data[] = {0x3,0x4,0x5};
            ccsds.setApplicationData( data,3);
            const auto primaryHeaderSize = ccsds.getPrimaryHeader64bit() & 0xFFFF;
            const auto dataFieldHeaderSize = ccsds.getDataFieldHeader()[7];

            return primaryHeaderSize == 0xB && dataFieldHeaderSize == 0x3;
        });

        tester->unitTest("Automatic data Length check with get full ccsds packet",[&ccsds] {

            const auto ccsdsPacket = ccsds.serialize();
            // data sizes:
            // Primary header 6 bytes (last byte for data field size includes data Field Header):
            //   Data Field Header 8 bytes (last byte for data field size):
            //      Application Data 3 byte
            // CRC 2 Bytes
            return ccsdsPacket[5] == 0xB && ccsdsPacket[5+8] == 0x3 && ccsdsPacket.size() == 6 + 8 + 3 + 2;
        });
    }

    tester->unitTest("PUS-A data field header assignment using vector buffer.",[] {
        std::vector<uint8_t> data = {0x1, 0x4, 0x5, 0x06, 0x07, 0xa};

        CCSDS::Packet packet;
        packet.setDataFieldHeader(data,CCSDS::PUS_A);
        auto dfh = packet.getDataFieldHeader();

        return  std::equal(data.begin(), data.end(), dfh.begin());
    });

    tester->unitTest("PUS-B data field header assignment using vector buffer.",[] {
        std::vector<uint8_t> data = {0x2, 0x4, 0x5, 0x06, 0x07, 0x0a, 0x0b, 0xc};

        CCSDS::Packet packet;
        packet.setDataFieldHeader(data,CCSDS::PUS_B);
        auto dfh = packet.getDataFieldHeader();

        return  std::equal(data.begin(), data.end(), dfh.begin());
    });

    tester->unitTest("PUS-C data field header assignment using vector buffer.",[] {
        std::vector<uint8_t> data = {0x3, 0x4, 0x5, 0x06, 0x0a, 0xb, 0xc, 0xd};

        CCSDS::Packet packet;
        packet.setDataFieldHeader(data,CCSDS::PUS_C);
        auto dfh = packet.getDataFieldHeader();

        return  std::equal(data.begin(), data.end(), dfh.begin());
    });

    tester->unitTest("PUS-A data field header assignment using uint8 buffer*, size and Type.",[] {
        constexpr uint8_t data[]= {0x1, 0x4, 0x5, 0x06, 0x07, 0xa};
        CCSDS::Packet packet;
        packet.setDataFieldHeader(data,6,CCSDS::PUS_A);
        auto dfh = packet.getDataFieldHeader();
        std::vector<uint8_t> tmp;
        tmp.assign(data,data+6);

        return  std::equal(tmp.begin(), tmp.end(), dfh.begin());
    });

    tester->unitTest("PUS-B data field header assignment using uint8 buffer*, size and Type.",[] {
        constexpr uint8_t data[] = {0x2, 0x4, 0x5, 0x06, 0x07, 0x0a, 0x0b, 0xc};
        CCSDS::Packet packet;
        packet.setDataFieldHeader(data,8,CCSDS::PUS_B);
        auto dfh = packet.getDataFieldHeader();
        std::vector<uint8_t> tmp;
        tmp.assign(data,data+8);

        return  std::equal(tmp.begin(), tmp.end(), dfh.begin());
    });

    tester->unitTest("PUS-C data field header assignment using uint8 buffer*, size and Type.",[] {
        constexpr uint8_t data[] = {0x3, 0x4, 0x5, 0x06, 0x0a, 0xb, 0xc, 0xd};
        CCSDS::Packet packet;
        packet.setDataFieldHeader(data,8,CCSDS::PUS_C);
        auto dfh = packet.getDataFieldHeader();
        std::vector<uint8_t> tmp;
        tmp.assign(data,data+8);

        return  std::equal(tmp.begin(), tmp.end(), dfh.begin());
    });

}


int main() {

    // use this to test some functionality




    // Perform unit tests.
    std::cout << std::endl;
    std::cout <<"Running Tests..." << std::endl;
    TestManager tester{};

    // groups of tests to perform.
    testGroupBasic(&tester, "Basic CCSDS Packet feature tests.");

    return tester.Result();
}

