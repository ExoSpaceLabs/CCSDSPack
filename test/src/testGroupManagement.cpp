#include <iostream>
#include "CCSDSManager.h"
#include "CCSDSUtils.h"
#include "CCSDSResult.h"
#include "tests.h"

void testGroupManagement(TestManager *tester, const std::string &description) {
  std::cout << "\n  testGroupManagement: " << description << std::endl;

  tester->unitTest("Manager shall set packet template, returned shall be as expected.", [] {
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

    tester->unitTest("Manager set data, returned packet shall be as expected.", [&manager] {
      manager.setDatFieldSize(5);
      TEST_VOID(manager.setApplicationData({0x01, 0x02, 0x03, 0x04, 0x05}));
      std::vector<uint8_t> ret{};
      TEST_RET(ret, manager.getPacketBufferAtIndex(0));
      std::vector<uint8_t> expected{0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04};
      const uint16_t totalNumberOfPackets = manager.getTotalPackets();

      return std::equal(expected.begin(), expected.end(), ret.begin()) && totalNumberOfPackets == 1;
    });
  }

  {
    // Note: Max data field size is set to 5 bytes, and header is already set.
    CCSDS::Packet newTestPacket{};
    ASSERT_SUCCESS(newTestPacket.setPrimaryHeader({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    CCSDS::Manager manager;
    ASSERT_SUCCESS(manager.setPacketTemplate(newTestPacket));
    manager.setDatFieldSize(5);
    tester->unitTest("Manager shall set large data for multi packets with sequence control.", [&manager] {

      TEST_VOID(manager.setApplicationData({0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}));

      std::vector<std::vector<uint8_t> > ret{};
      const uint16_t totalNumberOfPackets = manager.getTotalPackets();
      ret.reserve(totalNumberOfPackets);
      for (int i = 0; i < totalNumberOfPackets; i++) {
        std::vector<uint8_t> pack{};
        TEST_RET(pack, manager.getPacketBufferAtIndex(i));
        ret.push_back(pack);
      }

      std::vector<std::vector<uint8_t> > expected{
        {0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
        {0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
        {0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e}
      };
      return std::equal(expected.begin(), expected.end(), ret.begin()) && totalNumberOfPackets == 3;
    });

    tester->unitTest("Manager get application data, shall be the same as set data", [&manager] {
      std::vector<uint8_t> expected{0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
      std::vector<uint8_t> ret{};
      TEST_RET(ret, manager.getApplicationDataBuffer());
      return std::equal(expected.begin(), expected.end(), ret.begin());
    });

    tester->unitTest("Manager get application data at index, shall be the same as set data.", [&manager] {
      std::vector<std::vector<uint8_t> > ret{};
      std::vector<std::vector<uint8_t> > expected{
        {0x01, 0x02, 0x03, 0x04, 0x05},
        {0x01, 0x02, 0x03, 0x04, 0x05},
        {0x06, 0x07}
      };

      const uint16_t totalNumberOfPackets = manager.getTotalPackets();
      for (int i = 0; i < totalNumberOfPackets; i++) {
        std::vector<uint8_t> data{};
        TEST_RET(data, manager.getApplicationDataBufferAtIndex(i));
        ret.push_back(data);
      }
      return std::equal(expected.begin(), expected.end(), ret.begin());
    });

    tester->unitTest("Manager returned vector of Packets shall be as expected.", [&manager] {
      // Note: Max data field size is set to 5 bytes, and header is already set.
      std::vector<std::vector<uint8_t> > ret{};
      const std::vector<CCSDS::Packet> localPackets = manager.getPackets();
      ret.reserve(localPackets.size());
      for (auto localPacket: localPackets) {
        std::vector<uint8_t> pack{};
        pack = localPacket.serialize();
        //TEST_RET(pack, packet.serialize(); // future
        ret.push_back(pack);
      }

      std::vector<std::vector<uint8_t> > expected{
        {0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
        {0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
        {0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e}
      };

      return std::equal(expected.begin(), expected.end(), ret.begin()) && localPackets.size() == 3;
    });
  }

  tester->unitTest("Manager shall add a single packet.", [] {
    // Note: Max data field size is set to 5 bytes, and header is already set.
    CCSDS::Manager manager{};
    std::vector<std::vector<uint8_t> > expected{
      {0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
      {0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
      {0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e}
    };
    for (auto& data : expected) {
      CCSDS::Packet packet{};
      TEST_VOID(packet.deserialize(data));
      TEST_VOID(manager.addPacket(packet));
    }

    std::vector<std::vector<uint8_t> > ret{};
    const std::vector<CCSDS::Packet> localPackets = manager.getPackets();
    ret.reserve(localPackets.size());
    for (auto localPacket: localPackets) {
      std::vector<uint8_t> pack{};
      pack = localPacket.serialize();
      ret.push_back(pack);
    }
    return std::equal(expected.begin(), expected.end(), ret.begin()) && localPackets.size() == 3;
  });

  tester->unitTest("Manager shall add a single packets from buffer.", [] {
    // Note: Max data field size is set to 5 bytes, and header is already set.
    CCSDS::Manager manager{};
    std::vector<uint8_t> expected{0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04};
    TEST_VOID(manager.addPacketFromBuffer(expected));

    std::vector<uint8_t> ret{};
    TEST_RET(ret, manager.getPacketBufferAtIndex(0));

    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("Manager shall add a series of segmented single packets from buffer.", [] {
    // Note: Max data field size is set to 5 bytes, and header is already set.
    CCSDS::Manager manager{};
    std::vector<std::vector<uint8_t> > expected{
      {0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
      {0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
      {0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e}
    };
    for (auto& data : expected) {
      TEST_VOID(manager.addPacketFromBuffer(data));
    }

    std::vector<std::vector<uint8_t> > ret{};
    const std::vector<CCSDS::Packet> localPackets = manager.getPackets();
    ret.reserve(localPackets.size());
    for (auto localPacket: localPackets) {
      std::vector<uint8_t> pack{};
      pack = localPacket.serialize();
      ret.push_back(pack);
    }
    return std::equal(expected.begin(), expected.end(), ret.begin()) && localPackets.size() == 3;
  });

  tester->unitTest("Manager shall load a series of segmented packets.", [] {
    // Note: Max data field size is set to 5 bytes, and header is already set.
    CCSDS::Manager manager{};
    std::vector<std::vector<uint8_t> > expected{
      {0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
      {0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
      {0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e}
    };
    std::vector<CCSDS::Packet> packets;
    for (auto& data : expected) {
      CCSDS::Packet packet;
      TEST_VOID(packet.deserialize(data));
      packets.push_back(packet);
    }
    TEST_VOID(manager.load(packets));

    std::vector<std::vector<uint8_t> > ret{};
    const std::vector<CCSDS::Packet> localPackets = manager.getPackets();
    ret.reserve(localPackets.size());
    for (auto localPacket: localPackets) {
      std::vector<uint8_t> pack{};
      pack = localPacket.serialize();
      ret.push_back(pack);
    }
    return std::equal(expected.begin(), expected.end(), ret.begin()) && localPackets.size() == 3;
  });

  tester->unitTest("Manager shall load a series of segmented packets from buffer.", [] {
    // Note: Max data field size is set to 5 bytes, and header is already set.
    CCSDS::Manager manager{};
    std::vector<std::vector<uint8_t>> expected{
      {0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
      {0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04},
      {0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e}
    };
    const std::vector<uint8_t> buffer{
          0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
          0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
          0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e
        };

    TEST_VOID(manager.load(buffer));

    std::vector<std::vector<uint8_t> > ret{};
    const std::vector<CCSDS::Packet> localPackets = manager.getPackets();
    ret.reserve(localPackets.size());
    for (auto localPacket: localPackets) {
      std::vector<uint8_t> pack{};
      pack = localPacket.serialize();
      ret.push_back(pack);
    }
    return std::equal(expected.begin(), expected.end(), ret.begin()) && localPackets.size() == 3;
  });

  tester->unitTest("Manager shall return a series of segmented packets to a buffer.", [] {
    // Note: Max data field size is set to 5 bytes, and header is already set.
    CCSDS::Manager manager{};
    const std::vector<uint8_t> expected{
          0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
          0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
          0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e
        };

    TEST_VOID(manager.load(expected));

    auto ret = manager.getPacketsBuffer();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  {
    CCSDS::Manager manager{};
    const std::vector<uint8_t> buffer{
      0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
      0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
      0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e
    };
    ASSERT_SUCCESS(manager.load(buffer));

    tester->unitTest("Manager shall clear the managed packets.", [&manager] {
      manager.clearPackets();
      const auto ret = manager.getTotalPackets();
      const auto packets = manager.getPackets();
      return ret == 0 && packets.empty();
    });
  }

  {
    CCSDS::Manager manager{};
    const std::vector<uint8_t> buffer{
      0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
      0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
      0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e
    };
    ASSERT_SUCCESS(manager.load(buffer));

    tester->unitTest("Manager shall clear everything.", [&manager] {
      manager.clear();
      manager.setAutoUpdateEnable(false);
      const auto packets = manager.getPackets();
      std::vector<uint8_t> ret;
      std::vector<uint8_t> expected{0x00, 0x00, 0xc0, 0x00, 0x00 , 0x00, 0xff, 0xff};
      TEST_RET(ret, manager.getPacketTemplate());

      return packets.empty() && std::equal(expected.begin(), expected.end(), ret.begin());
    });
    {
      CCSDS::Manager manager1{};
      const std::vector<uint8_t> expected{
        0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
        0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
        0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e
      };
      ASSERT_SUCCESS(manager1.load(expected));

      tester->unitTest("Manager shall write the packets to a binary file successfully.", [&manager1] {
        bool ret;
        TEST_RET(ret, manager1.write("myPackets.bin"));
        return ret;
      });
    }

    {
      CCSDS::Manager manager1{};
      const std::vector<uint8_t> expected{
        0xF7, 0xFF, 0x40, 0x01, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
        0xF7, 0xFF, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04,
        0xF7, 0xFF, 0x80, 0x03, 0x00, 0x02, 0x06, 0x07, 0xc7, 0x4e
      };
      tester->unitTest("Manager shall read packets from a binary file successfully.", [&manager1, &expected] {
        bool ret;
        TEST_RET(ret, manager1.read("myPackets.bin"));
        auto retPackets = manager1.getPacketsBuffer();
        return ret && std::equal(expected.begin(), expected.end(), retPackets.begin());
      });

    }

    tester->unitTest("Manager shall load template from binary file, template shall be as expected.", [] {
      CCSDS::Packet packet{};
      std::vector<uint8_t> expected{0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00, 0xff, 0xff};
      CCSDS::Manager manager;
      TEST_VOID(manager.readTemplate("templatePacket.bin"));
      std::vector<uint8_t> templatePacket;
      TEST_RET(templatePacket, manager.getPacketTemplate());
      return std::equal(expected.begin(), expected.end(), templatePacket.begin());
    });

    tester->unitTest("Manager shall load template from config file, template shall be as expected.", [] {
      CCSDS::Packet packet{};
      std::vector<uint8_t> expected{0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00, 0xff, 0xff};
      CCSDS::Manager manager;
      TEST_VOID(manager.readTemplate("templatePacket.cfg"));
      std::vector<uint8_t> templatePacket;
      TEST_RET(templatePacket, manager.getPacketTemplate());
      return std::equal(expected.begin(), expected.end(), templatePacket.begin());
    });
  }
}
