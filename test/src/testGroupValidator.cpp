#include <CCSDSValidator.h>
#include <iostream>
#include "CCSDSUtils.h"
#include "CCSDSResult.h"
#include "tests.h"

void testGroupValidator(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupValidator: " << description << std::endl;

  tester->unitTest("Validator UNSEGMENTED Packet coherence shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    CCSDS::Packet packet;
    TEST_VOID(packet.deserialize(packetData));
    return validator.validate(packet);
  });

  tester->unitTest("Validator UNSEGMENTED Packet coherence shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x00, 0x5, 0x01, 0x02, 0x03, 0x04, 0x05, 0x97,
      0x7d, 0x01, 0x02
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return !validator.validate(packet);
  });

  tester->unitTest("Validator SEGMENTED Packet coherence shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0x40, 0x01, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return validator.validate(packet);
  });

  tester->unitTest("Validator SEGMENTED Packet coherence shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0x40, 0x00, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return !validator.validate(packet);
  });


  tester->unitTest("Validator SEGMENTED Packet count coherence shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0x40, 0x01, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return validator.validate(packet);
  });

  tester->unitTest("Validator SEGMENTED Packet count coherence shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, false);
    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0x40, 0x03, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return !validator.validate(packet);
  });


  tester->unitTest("Validator UNSEGMENTED Packet against Template shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(false, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return validator.validate(packet);
  });

  tester->unitTest("Validator UNSEGMENTED Packet against Template shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(false, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    CCSDS::Packet packet;
    templatePacket.setUpdatePacketEnable(false);
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return !validator.validate(packet);
  });

  tester->unitTest("Validator SEGMENTED Packet against Template shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(false, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0x40, 0x05, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return validator.validate(packet);
  });

  tester->unitTest("Validator SEGMENTED Packet against Template shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(false, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xFF, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf
    };
    CCSDS::Packet packet;
    templatePacket.setUpdatePacketEnable(false);
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return !validator.validate(packet);
  });

  tester->unitTest("Validator UNSEGMENTED Packet coherence & against template shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return validator.validate(packet);
  });


  tester->unitTest("Validator UNSEGMENTED Packet coherence & against template shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0xc0, 0x05, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return !validator.validate(packet);
  });

  tester->unitTest("Validator SEGMENTED Packet coherence & against template shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0x40, 0x01, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return validator.validate(packet);
  });

  tester->unitTest("Validator SEGMENTED Packet coherence & against template shall fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0x40, 0x00, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    return !validator.validate(packet);
  });

  tester->unitTest("Validator SEGMENTED Packets sequence shall pass.", []() {
  CCSDS::Validator validator;
  validator.configure(true, true, true);

  CCSDS::Packet templatePacket;
  templatePacket.setUpdatePacketEnable(false);
  TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
  validator.setTemplatePacket(templatePacket);

  CCSDS::Packet packet1, packet2;
  packet1.setUpdatePacketEnable(false);
    TEST_VOID(packet1.deserialize({0xF7, 0xFF, 0x40, 0x01, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e}));
    TEST_VOID(packet2.deserialize({0xF7, 0xFF, 0x00, 0x02, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e}));
  return validator.validate(packet1) && validator.validate(packet2);
});

  tester->unitTest("Validator SEGMENTED Packets sequence shall fail.", []() {
  CCSDS::Validator validator;
  validator.configure(true, true, true);

  CCSDS::Packet templatePacket;
  templatePacket.setUpdatePacketEnable(false);
  TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
  validator.setTemplatePacket(templatePacket);

  CCSDS::Packet packet1, packet2;
  packet1.setUpdatePacketEnable(false);
    TEST_VOID(packet1.deserialize({0xF7, 0xFF, 0x40, 0x01, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e}));
    TEST_VOID(packet2.deserialize({0xF7, 0xFF, 0x00, 0x03, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e}));
  return validator.validate(packet1) && !validator.validate(packet2);
});

  tester->unitTest("Validator report, shall pass.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    std::vector<bool> expected{true, true, true, true, true, true};
    validator.validate(packet);
    auto report = validator.getReport();
    return std::equal(expected.begin(), expected.end(), report.begin());;
  });

  tester->unitTest("Validator report, shall fail on data field length.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x0d, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    std::vector<bool> expected{false, true, true, true, true, true};
    validator.validate(packet);
    auto report = validator.getReport();
    return std::equal(expected.begin(), expected.end(), report.begin());;
  });


  tester->unitTest("Validator report, shall fail on CRC.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1d
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    std::vector<bool> expected{true, false, true, true, true, true};
    validator.validate(packet);
    auto report = validator.getReport();
    return std::equal(expected.begin(), expected.end(), report.begin());;
  });

  tester->unitTest("Validator report, shall fail on sequence control.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0xc0, 0x05, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    std::vector<bool> expected{true, true, false, true, true, true};
    validator.validate(packet);
    auto report = validator.getReport();
    return std::equal(expected.begin(), expected.end(), report.begin());;
  });

  tester->unitTest("Validator report, shall fail on sequence control counter.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0x40, 0x05, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    std::vector<bool> expected{true, true, true, false, true, true};
    validator.validate(packet);
    auto report = validator.getReport();
    return std::equal(expected.begin(), expected.end(), report.begin());;
  });

  tester->unitTest("Validator report, shall fail on Identification and version.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0x5F, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    std::vector<bool> expected{true, true, true, true, false, true};
    validator.validate(packet);
    auto report = validator.getReport();
    return std::equal(expected.begin(), expected.end(), report.begin());;
  });

  tester->unitTest("Validator report, shall fail on header flag.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xFF, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    std::vector<bool> expected{true, true, true, true, true, false};
    validator.validate(packet);
    auto report = validator.getReport();
    return std::equal(expected.begin(), expected.end(), report.begin());;
  });

  tester->unitTest("Validator report, shall fully fail.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    TEST_VOID(templatePacket.setPrimaryHeader({0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    const std::vector<uint8_t> packetData{
      0xF7, 0xFF, 0x40, 0x00, 0x00, 0x05, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1d
    };
    CCSDS::Packet packet;
    packet.setUpdatePacketEnable(false);
    TEST_VOID(packet.deserialize(packetData));
    std::vector<bool> expected{false, false, false, false, false, false};
    validator.validate(packet);
    auto report = validator.getReport();
    return std::equal(expected.begin(), expected.end(), report.begin());;
  });


  tester->unitTest("Validator shall clear its variables.", []() {
    CCSDS::Validator validator;
    validator.configure(true, true, true);

    CCSDS::Packet templatePacket;
    templatePacket.setUpdatePacketEnable(false);
    TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0x40, 0x00, 0x00, 0x00}));
    validator.setTemplatePacket(templatePacket);

    CCSDS::Packet packet1, packet2;
    packet1.setUpdatePacketEnable(false);
    packet2.setUpdatePacketEnable(false);
    TEST_VOID(packet1.deserialize({0xF7, 0xFF, 0x40, 0x01, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e}));
    TEST_VOID(packet2.deserialize({0xF7, 0xFF, 0x00, 0x02, 0x00, 0x0c, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05,
      0xd9, 0x1e}));
    validator.validate(packet1);
    validator.validate(packet2);
    validator.clear();
    validator.setTemplatePacket(templatePacket);
  return validator.validate(packet1) && validator.validate(packet2);
});
  std::cout << std::endl;
}
