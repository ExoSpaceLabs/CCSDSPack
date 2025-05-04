//
// Created by inczert on 3/1/25.
//

#include <CCSDSValidator.h>
#include <iostream>
#include "CCSDSUtils.h"
#include "CCSDSResult.h"
#include "tests.h"
#include "PusServices.h"

void testGroupCore(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupCore: " << description << std::endl;

  tester->unitTest("Assign Primary header unsegmented using an uint64_t as input.", []() {
    constexpr uint64_t headerData(0xFFFFFFFFFFFF);
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(headerData));
    // getPrimaryHeader updated dependent fields to correct values.
    const auto ret = packet.getPrimaryHeader64bit();
    return ret == 0xf7ffc0000000;
  });

  tester->unitTest("Assign Primary header segmented using an uint64_t as input.", []() {
    constexpr uint64_t headerData(0xF7FF4FFFFFFF);
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(headerData));
    // getPrimaryHeader updated dependent fields to correct values.
    const auto ret = packet.getPrimaryHeader64bit();
    return ret == 0xf7ff4FFF0000;
  });

  tester->unitTest("Assign Primary header using PrimaryHeader struct as input.", []() {
    constexpr uint64_t expectedHeaderData(0x300140010000);
    const CCSDS::PrimaryHeader headerData(1,
                                          1,
                                          1,
                                          1,
                                          1,
                                          1,
                                          1);
    CCSDS::Packet packet;
    packet.setPrimaryHeader(headerData);
    const auto ret = packet.getPrimaryHeader64bit();
    return ret == expectedHeaderData;
  });

  tester->unitTest("Assign Primary header performing vector deserialization.", []() {
    constexpr uint64_t expectedHeaderData(0xf7ffc0000000);
    const std::vector<uint8_t> data({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(data));
    const auto ret = packet.getPrimaryHeader64bit();
    return ret == expectedHeaderData;
  });

  tester->unitTest("Get Header using vector serialization.", []() {
    constexpr uint64_t data(0xf7ffc0000000);
    const std::vector<uint8_t> expectedHeaderData({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00});
    CCSDS::Packet packet;
    TEST_VOID(packet.setPrimaryHeader(data));
    const auto ret = packet.getPrimaryHeaderBytes();
    return std::equal(expectedHeaderData.begin(), expectedHeaderData.end(), ret.begin());
  }); {
    CCSDS::Packet packet;

    tester->unitTest("Assign Data field using vector, DataFieldHeader shall be empty.", [&packet]() {
      TEST_VOID(packet.setApplicationData({1, 2, 3, 4, 5}));
      const auto dfh = packet.getDataFieldHeaderBytes();
      return dfh.empty();
    });

    tester->unitTest("Assign Data field using vector, ApplicationData shall be of correct size.", [&packet] {
      const auto apd = packet.getApplicationDataBytes();
      return apd.size() == 5;
    });

    tester->unitTest("Assign Data field using vector, CRC16 shall be correct.", [&packet]() {
      constexpr uint16_t expectedCRC16(0x9304);
      const auto crc(packet.getCRC());
      return crc == expectedCRC16;
    });
  } {
    CCSDS::Packet packet;

    tester->unitTest("Assign Secondary Header and Data using vector, DataFieldHeader shall be of correct size.",
                     [&packet] {
                       TEST_VOID(packet.setDataFieldHeader({0x1,0x2,0x3}));
                       TEST_VOID(packet.setApplicationData({4, 5}));
                       const auto dfh = packet.getDataFieldHeaderBytes();
                       return dfh.size() == 3;
                     });

    tester->unitTest("Assign Secondary Header and Data using vector, ApplicationData, shall be of correct size.",
                     [&packet] {
                       const auto apd = packet.getApplicationDataBytes();
                       return apd.size() == 2;
                     });


    tester->unitTest("Assign Secondary Header and Data using vector, CRC16 shall be correct.", [&packet] {
      constexpr uint16_t expectedCRC16(0x9304);
      const auto crc(packet.getCRC());
      return crc == expectedCRC16;
    });
  } {
    CCSDS::Packet packet;

    tester->unitTest("Assign Data field using array*, DataFieldHeader shall be empty.", [&packet] {
      const uint8_t data[] = {0x1, 0x2, 0x3, 0x4, 0x5};
      TEST_VOID(packet.setApplicationData( data,5));
      const auto dfh = packet.getDataFieldHeaderBytes();
      return dfh.empty();
    });

    tester->unitTest("Assign Data field using array*, ApplicationData shall be of correct size.", [&packet] {
      const auto apd = packet.getApplicationDataBytes();
      return apd.size() == 5;
    });

    tester->unitTest("Assign Data field using array*, CRC16 shall be correct.", [&packet] {
      constexpr uint16_t expectedCRC16(0x9304);
      const auto crc(packet.getCRC());
      return crc == expectedCRC16;
    });
  } {
    CCSDS::Packet packet;

    tester->unitTest("Primary header vector shall be default valued.", [&packet] {
      // although the header is set to FFFF for data field size, its content is updated by data field size.
      std::vector<uint8_t> expectedHeader{0x0, 0x0, 0xc0, 0x0, 0x0, 0x0};
      const auto header = packet.getPrimaryHeaderBytes();
      return std::equal(expectedHeader.begin(), expectedHeader.end(), header.begin());
    });
    tester->unitTest("Assign Secondary header and data field using array*, DataFieldHeader shall be of correct size.",
                     [&packet] {
                       constexpr uint8_t secondaryHeader[] = {0x1, 0x2};
                       constexpr uint8_t data[] = {0x3, 0x4, 0x5};
                       TEST_VOID(packet.setApplicationData( data,3));
                       TEST_VOID(packet.setDataFieldHeader( secondaryHeader, 2));
                       const auto dfh = packet.getDataFieldHeaderBytes();
                       return dfh.size() == 2;
                     });

    tester->unitTest("Assign Secondary header and data field using array*, ApplicationData shall be of correct size.",
                     [&packet] {
                       const auto apd = packet.getApplicationDataBytes();
                       return apd.size() == 3;
                     });

    tester->unitTest("Assign Secondary header and data field using array*, CRC16 shall be correct.", [&packet] {
      constexpr uint16_t expectedCRC16(0x9304);
      const auto crc(packet.getCRC());
      return crc == expectedCRC16;
    });

    tester->unitTest("Primary header vector getter values shall be correctly returned.", [&packet] {
      TEST_VOID(packet.setPrimaryHeader(0xffffffffffff));
      // although the header is set to FFFF for data field size, its content is Header getters.
      const std::vector<uint8_t> expectedHeader = {
        0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x05
      };
      const auto header = packet.getPrimaryHeaderBytes();
      return std::equal(expectedHeader.begin(), expectedHeader.end(), header.begin());
    });

    tester->unitTest("CRC16 vector getter values shall be correctly returned.", [&packet] {
      const auto crc = packet.getCRCVectorBytes();
      auto res(true);
      res &= crc[0] == 0x93;
      res &= crc[1] == 0x04;;
      return res;
    });

    tester->unitTest("Get full packet size. Header, Data field and CRC shall be correctly positioned.", [&packet] {
      const auto header = packet.getPrimaryHeaderBytes();
      const auto data = packet.getFullDataFieldBytes();
      const size_t packetSize = 6 + 2 + data.size();
      const auto pack = packet.serialize();
      bool res(pack.size() == packetSize);
      res &= header[3] == pack[3];
      res &= pack[6] == data[0];
      res &= pack[10] == data[4];
      const auto crc = packet.getCRCVectorBytes();
      res &= crc[0] == pack[packetSize - 2];
      res &= crc[1] == pack[packetSize - 1];;
      return res;
    });
  }

  tester->unitTest("PUS-A data field header assignment using vector buffer.", [] {
    std::vector<uint8_t> expected = {0x1, 0x4, 0x5, 0x06, 0x07, 0xa};

    CCSDS::Packet packet;
    TEST_VOID(packet.setDataFieldHeader(expected,"PusA"));
    packet.setUpdatePacketEnable(false);
    auto ret = packet.getDataFieldHeaderBytes();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("PUS-B data field header assignment using vector buffer.", [] {
    std::vector<uint8_t> data = {0x2, 0x4, 0x5, 0x06, 0x07, 0x0a, 0x0b, 0xc};

    CCSDS::Packet packet;
    TEST_VOID(packet.setDataFieldHeader(data,"PusB"));
    packet.setUpdatePacketEnable(false);
    auto dfh = packet.getDataFieldHeaderBytes();

    return std::equal(data.begin(), data.end(), dfh.begin());
  });

  tester->unitTest("PUS-C data field header assignment using vector buffer.", [] {
    std::vector<uint8_t> data = {0x3, 0x4, 0x5, 0x06, 0x0a, 0xb, 0xc, 0xd};

    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.setDataFieldHeader(data,"PusC"));
    auto dfh = packet.getDataFieldHeaderBytes();

    return std::equal(data.begin(), data.end(), dfh.begin());
  });

  tester->unitTest("PUS-A data field header assignment using uint8 buffer*, size and Type.", [] {
    constexpr uint8_t data[] = {0x1, 0x4, 0x5, 0x06, 0x07, 0xa};
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.setDataFieldHeader(data,6,"PusA"));
    auto dfh = packet.getDataFieldHeaderBytes();
    std::vector<uint8_t> tmp;
    tmp.assign(data, data + 6);

    return std::equal(tmp.begin(), tmp.end(), dfh.begin());
  });

  tester->unitTest("PUS-B data field header assignment using uint8 buffer*, size and Type.", [] {
    constexpr uint8_t data[] = {0x2, 0x4, 0x5, 0x06, 0x07, 0x0a, 0x0b, 0xc};
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.setDataFieldHeader(data,8,"PusB"));
    auto dfh = packet.getDataFieldHeaderBytes();
    std::vector<uint8_t> tmp;
    tmp.assign(data, data + 8);

    return std::equal(tmp.begin(), tmp.end(), dfh.begin());
  });

  tester->unitTest("PUS-C data field header assignment using uint8 buffer*, size and Type.", [] {
    constexpr uint8_t data[] = {0x3, 0x4, 0x5, 0x06, 0x0a, 0xb, 0xc, 0xd};
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.setDataFieldHeader(data,8,"PusC"));
    auto dfh = packet.getDataFieldHeaderBytes();
    std::vector<uint8_t> tmp;
    tmp.assign(data, data + 8);

    return std::equal(tmp.begin(), tmp.end(), dfh.begin());
  });

  tester->unitTest("PUS-A secondary header assignment. returned data field header size is of correct size of 6 bytes.",
                   [] {
                     CCSDS::Packet packet;
                     //const uint8_t data[] = {0x1,0x2,0x3,0x4,0x5};
                     PusA pusAHeader(1, 2, 3, 4, 5);
                     const auto ptr = std::make_shared<PusA>(pusAHeader);
                     packet.setDataFieldHeader(ptr);
                     const auto dfh = packet.getDataFieldHeaderBytes();
                     return dfh.size() == pusAHeader.getSize();
                   });

  tester->unitTest("PUS-B secondary header assignment. returned data field header size is of correct size of 8 bytes.",
                   [] {
                     CCSDS::Packet packet;
                     PusB pusBHeader(1, 2, 3, 4, 5, 6);
                     auto ptr = std::make_shared<PusB>(pusBHeader);
                     packet.setDataFieldHeader(ptr);
                     const auto dfh = packet.getDataFieldHeaderBytes();
                     return dfh.size() == pusBHeader.getSize();
                   }); {
    CCSDS::Packet packet;

    tester->unitTest(
      "PUS-C secondary header assignment. returned data field header size is of correct size of 8 bytes.", [&packet] {
        const PusC pusCHeader(1, 2, 3, 4, 5, 6);
        auto ptr = std::make_shared<PusC>(pusCHeader);
        packet.setDataFieldHeader(ptr);
        const auto dfh = packet.getDataFieldHeaderBytes();
        return dfh.size() == pusCHeader.getSize();
      });

    tester->unitTest("Automatic data Length update in Primary header (PUS-C size).", [&packet] {
      //ccsds.setPrimaryHeader(0xffffffff);
      //const uint8_t data[] = {0x1,0x2,0x3,0x4,0x5};
      const auto primaryHeader = packet.getPrimaryHeader64bit();
      return (primaryHeader & 0xFFFF) == 0x8;
    });

    tester->unitTest("Automatic data Length update in Primary header and Secondary header after data inclusion.",
                     [&packet] {
                       constexpr uint8_t data[] = {0x3, 0x4, 0x5};
                       TEST_VOID(packet.setApplicationData( data,3));
                       const auto primaryHeaderSize = packet.getPrimaryHeader64bit() & 0xFFFF;
                       const auto dataFieldHeader = packet.getDataFieldHeaderBytes();
                       const auto dataFieldHeaderSize = dataFieldHeader[7];
                       return primaryHeaderSize == 0xB && dataFieldHeaderSize == 0x3;
                     });

    tester->unitTest("Automatic data Length check with get full ccsds packet.", [&packet] {
      const auto ret = packet.serialize();

      // data sizes:
      // Primary header 6 bytes (last byte for data field size includes data Field Header):
      //   Data Field Header 8 bytes (last byte for data field size):
      //      Application Data 3 byte
      // CRC 2 Bytes
      return ret[5] == 0xB && ret[5 + 8] == 0x3 && ret.size() == 6 + 8 + 3 + 2;
    });
  }

  tester->unitTest("Deserialize vector data to header and application data and serialize it.", [] {
    CCSDS::Packet packet;
    // note when using deserialize the header sequence flag stays the same, therefore make sure  it is correct.
    std::vector<uint8_t> expected{0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04};
    TEST_VOID(packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x02}));
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("Deserialize vector data to header, secondary header and application data and serialize it.", [] {
    CCSDS::Packet packet;
    std::vector<uint8_t> expected{0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04};
    TEST_VOID(packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x02}, 2));
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("Deserialize vector data to header, PUS-A and application data and serialize it.", [] {
    CCSDS::Packet packet;
    std::vector<uint8_t> expected{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x00, 0x5, 0x01, 0x02, 0x03, 0x04, 0x05, 0x97, 0x7d
    };
    TEST_VOID(
      packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x01, 0x02}, "PusA"));
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("Deserialize vector data to header, PUS-B and application data and serialize it.", [] {
    CCSDS::Packet packet;
    std::vector<uint8_t> expected{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    TEST_VOID(
      packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x01, 0x02}, "PusB"));
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("Deserialize vector data to header, PUS-C and application data and serialize it.", [] {
    CCSDS::Packet packet;
    std::vector<uint8_t> expected{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    TEST_VOID(
      packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x01, 0x02}, "PusC"));
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest(
    "Construct Packet using vector data to header, secondary header and application data and serialize it.", [] {
      std::vector<uint8_t> expected{0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04};
      CCSDS::Packet packet;
      TEST_VOID(packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x01, 0x02}, 2 ));

      auto ret = packet.serialize();
      return std::equal(expected.begin(), expected.end(), ret.begin());
    });

  tester->unitTest("Construct Packet using vector data to header, PUS-A and application data and serialize it.", [] {
    CCSDS::Packet packet;
    TEST_VOID(
      packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x01, 0x02}, "PusA"));
    std::vector<uint8_t> expected{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x00, 0x5, 0x01, 0x02, 0x03, 0x04, 0x05, 0x97, 0x7d
    };
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("Construct Packet using vector data to header, PUS-B and application data and serialize it.", [] {
    CCSDS::Packet packet;
    TEST_VOID(
      packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x01, 0x02}, "PusB"));
    std::vector<uint8_t> expected{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("Construct Packet using vector data to header, PUS-C and application data and serialize it.", [] {
    CCSDS::Packet packet;
    TEST_VOID(
      packet.deserialize({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x01, 0x02, 0x03, 0x04,
        0x05, 0x01, 0x02}, "PusC"));
    std::vector<uint8_t> expected{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest("Construct Packet disable auto update, returned data shall be as set.", [] {
    std::vector<uint8_t> expected{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x93, 0x04, 0x01, 0x02
    };
    CCSDS::Packet packet;
    TEST_VOID(packet.deserialize(expected, 2));
    packet.setUpdatePacketEnable(false);
    auto ret = packet.serialize();
    return std::equal(expected.begin(), expected.end(), ret.begin());
  });

  tester->unitTest(
    "Construct Packet using vector data to header, PUS-A and application data, disable auto update, returned data shall be as set.",
    [] {
      std::vector<uint8_t> expected{
        0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x00, 0x5, 0x01, 0x02, 0x03, 0x04, 0x05, 0x97, 0x7d,
        0x01, 0x02
      };
      CCSDS::Packet packet;
      packet.setUpdatePacketEnable(false);
      TEST_VOID(packet.deserialize(expected, "PusA"));
      auto ret = packet.serialize();
      return std::equal(expected.begin(), expected.end(), ret.begin());
    });

  tester->unitTest(
    "Construct Packet using vector data to header, PUS-B and application data, disable auto update, returned data shall be as set.",
    [] {
      std::vector<uint8_t> expected{
        0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf,
        0x01, 0x02
      };
      CCSDS::Packet packet;
      packet.setUpdatePacketEnable(false);
      TEST_VOID(packet.deserialize(expected, "PusB"));
      auto ret = packet.serialize();
      return std::equal(expected.begin(), expected.end(), ret.begin());
    });

  tester->unitTest(
    "Construct Packet using vector data to header, PUS-C and application data, disable auto update, returned data shall be as set.",
    [] {
      std::vector<uint8_t> expected{
        0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf,
        0x01, 0x02
      };
      CCSDS::Packet packet;
      packet.setUpdatePacketEnable(false);
      TEST_VOID(packet.deserialize(expected, "PusC"));
      auto ret = packet.serialize();
      return std::equal(expected.begin(), expected.end(), ret.begin());
    });

  tester->unitTest("Write binary data to a file.",[] {
      const std::vector<uint8_t> data{
        0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf,
        0x01, 0x02, 0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
        0x97, 0xdf, 0x01, 0x02, 0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03,
        0x04, 0x05, 0x97, 0xdf,0x01, 0x02, 0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00,
        0x03, 0x03, 0x04, 0x05, 0x97, 0xdf, 0x01, 0x02
      };
      bool ret{false};
      TEST_RET(ret, writeBinaryFile(data, "test_resources/binaryFile.bin"));
      return ret;
    });

  tester->unitTest("Read binary data from file.",[] {
      std::vector<uint8_t> expected{
        0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf,
        0x01, 0x02, 0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
        0x97, 0xdf, 0x01, 0x02, 0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03,
        0x04, 0x05, 0x97, 0xdf,0x01, 0x02, 0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00,
        0x03, 0x03, 0x04, 0x05, 0x97, 0xdf, 0x01, 0x02
      };
      std::vector<uint8_t> ret;
      TEST_RET(ret, readBinaryFile("test_resources/binaryFile.bin"));
      return std::equal(expected.begin(), expected.end(), ret.begin());
    });

  tester->unitTest("Read data from configure file.",[] {
    Config cfg;
    bool ret{false};
    TEST_RET(ret, cfg.load("test_resources/test_config.cfg"));

    std::string stringValue;
    TEST_RET(stringValue,cfg.get<std::string>("stringValue"));
    if (stringValue != "Awsome string to test" ) return false;

    int integerValue;
    TEST_RET(integerValue,cfg.get<int>("integerValue"));
    std::cout << integerValue << std::endl;
    if (integerValue != 42 ) return false;

    bool booleanValue;
    TEST_RET(booleanValue,cfg.get<bool>("booleanValue"));
    if (booleanValue != true) return false;

    float floatValue;
    TEST_RET(floatValue,cfg.get<float>("floatValue"));
    if (floatValue != 0.85F ) return false;

    std::vector<uint8_t> buffer;
    TEST_RET(buffer, cfg.get<std::vector<uint8_t> >("buffer"));
    if (buffer != std::vector<uint8_t>{1,2,3,4,5} ) return false;

    std::vector<uint8_t> bufferHex;
    TEST_RET(bufferHex, cfg.get<std::vector<uint8_t> >("bufferHex"));
    if (bufferHex != std::vector<uint8_t>{0xFF,0xaa,0xBB,0xcc,0xEE}) return false;

    std::vector<uint8_t> bufferHex2;
    TEST_RET(bufferHex2, cfg.get<std::vector<uint8_t> >("bufferHex2"));
    if (bufferHex2 != std::vector<uint8_t>{0x2,0x40,0x56,0x87,0xf0} ) return false;

    return ret;
    });

  std::cout << std::endl;
}
