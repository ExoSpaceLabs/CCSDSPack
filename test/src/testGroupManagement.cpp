//
// Created by inczert on 3/1/25.
//

#include "tests.h"

void testGroupManagement(TestManager *tester, const std::string& description) {
    std::cout << "\n  testGroupManagement: " << description <<  std::endl;

    tester->unitTest("Create an instance of management and set packet template.",[] {
        CCSDS::Packet packet{};
        std::vector<uint8_t> expected{0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00, 0xff, 0xff};
        TEST_VOID(packet.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
        CCSDS::Manager manager(packet);
        std::vector<uint8_t> templatePacket;
        TEST_RET(templatePacket, manager.getPacketTemplate());
        return std::equal(expected.begin(), expected.end(), templatePacket.begin());
    });

    {
        CCSDS::Packet packet{};
        ASSERT_SUCCESS(packet.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
        CCSDS::Manager manager(packet);

        tester->unitTest("Manager set data and check returned packet.",[&manager] {
            manager.setDatFieldSize(5);
            TEST_VOID(manager.setApplicationData({0x01, 0x02, 0x03, 0x04, 0x05}));
            std::vector<uint8_t> ret{};
            TEST_RET(ret, manager.getPacketAtIndex(0));
            std::vector<uint8_t> expected{0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04};
            const uint16_t totalNumberOfPackets = manager.getTotalPackets();

            return std::equal(expected.begin(), expected.end(), ret.begin()) && totalNumberOfPackets == 1;
        });

        tester->unitTest("Manager set large data and check returned multi packets with sequence control.",[&manager] {
            // Note: Max data field size is set to 5 bytes, and header is already set.
            TEST_VOID(manager.setApplicationData({0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}));
            std::vector<std::vector<uint8_t>> ret{};
            const uint16_t totalNumberOfPackets =  manager.getTotalPackets();
            ret.reserve(totalNumberOfPackets);
            for (int i = 0; i < totalNumberOfPackets; i++) {
                std::vector<uint8_t> pack{};
                TEST_RET(pack, manager.getPacketAtIndex(i));
                ret.push_back(pack);
            }

            std::vector<std::vector<uint8_t>> expected{
                {0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
                {0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
                {0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e}};

            return std::equal(expected.begin(), expected.end(), ret.begin()) && totalNumberOfPackets == 3;
        });

        tester->unitTest("Manager get application data, shall be the same as set data",[&manager] {
            std::vector<uint8_t> expected {0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
            std::vector<uint8_t> ret{};
            TEST_RET(ret, manager.getApplicationData());
            return std::equal(expected.begin(), expected.end(), ret.begin());
        });

        tester->unitTest("Manager get application data at index, shall be the same as set data.",[&manager] {
            std::vector<std::vector<uint8_t>> ret{};
            std::vector<std::vector<uint8_t>> expected{
                { 0x01, 0x02, 0x03, 0x04, 0x05 },
                { 0x01, 0x02, 0x03, 0x04, 0x05 },
                { 0x06, 0x07 }};

            const uint16_t totalNumberOfPackets = manager.getTotalPackets();
            for (int i = 0; i < totalNumberOfPackets; i++) {
                std::vector<uint8_t> data{};
                TEST_RET(data, manager.getApplicationDataAtIndex(i));
                ret.push_back(data);
            }
            return std::equal(expected.begin(), expected.end(), ret.begin());
        });

        tester->unitTest("Manager returned vector of Packets shall be as expected.",[&manager] {
            // Note: Max data field size is set to 5 bytes, and header is already set.
            std::vector<std::vector<uint8_t>> ret{};
            const std::vector<CCSDS::Packet> localPackets = manager.getPackets();
            ret.reserve(localPackets.size());
            for (auto localPacket : localPackets) {
                std::vector<uint8_t> pack{};
                pack = localPacket.serialize();
                //TEST_RET(pack, packet.serialize(); // future
                ret.push_back(pack);
            }

            std::vector<std::vector<uint8_t>> expected{
                {0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
                {0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
                {0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e}};

            return std::equal(expected.begin(), expected.end(), ret.begin()) && localPackets.size() == 3;
        });
    }
}
