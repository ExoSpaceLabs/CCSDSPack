

#include <CCSDSValidator.h>
#include <iostream>
#include "CCSDSUtils.h"
#include "CCSDSResult.h"
#include "tests.h"

void testGroupValidator(TestManager *tester, const std::string& description) {
    std::cout << "\n  testGroupValidator: " << description <<  std::endl;

    tester->unitTest( "Validator Check Packet Coherence shall pass.", []() {
        CCSDS::Validator validator;
        validator.configure(true, false);
        const std::vector<uint8_t> expected{0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf};
        CCSDS::Packet packet;
        TEST_VOID(packet.deserialize(expected));
        return validator.validate(packet);
    });

    tester->unitTest( "Validator Check Packet Coherence shall fail.", []() {
        CCSDS::Validator validator;
        validator.configure(true, false);
        const std::vector<uint8_t> expected{0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x00, 0x5, 0x01, 0x02, 0x03, 0x04, 0x05, 0x97, 0x7d, 0x01, 0x02};
        CCSDS::Packet packet;
        packet.setUpdatePacketEnable(false);
        TEST_VOID(packet.deserialize(expected));
        return !validator.validate(packet);
    });


    tester->unitTest( "Validator Check Packet against Template shall pass.", []() {
        CCSDS::Validator validator;
        validator.configure(false, true);

        CCSDS::Packet templatePacket;
        TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
        validator.setTemplatePacket(templatePacket);

        const std::vector<uint8_t> expected{0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf};
        CCSDS::Packet packet;
        TEST_VOID(packet.deserialize(expected));
        return validator.validate(packet);
    });

    tester->unitTest( "Validator Check Packet against Template shall fail.", []() {
        CCSDS::Validator validator;
        validator.configure(false, true);

        CCSDS::Packet templatePacket;
        TEST_VOID(templatePacket.setPrimaryHeader({0xF7, 0xFF, 0xc0, 0x00, 0x00, 0x00}));
        validator.setTemplatePacket(templatePacket);

        const std::vector<uint8_t> expected{0xFF, 0xFF, 0xc0, 0x00, 0x00, 0x0b, 0x1, 0x4, 0x5, 0x06, 0x07, 0xa, 0x00, 0x03, 0x03, 0x04, 0x05, 0x97, 0xdf};
        CCSDS::Packet packet;
        packet.setUpdatePacketEnable(false);
        TEST_VOID(packet.deserialize(expected));
        return !validator.validate(packet);
    });
}
