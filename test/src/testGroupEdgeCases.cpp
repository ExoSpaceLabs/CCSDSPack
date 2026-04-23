#include <CCSDSValidator.h>
#include <iostream>
#include <memory>
#include "CCSDSUtils.h"
#include "CCSDSConfig.h"
#include "CCSDSResult.h"
#include "tests.h"
#include "PusServices.h"

void testGroupEdgeCases(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupEdgeCases: " << description << std::endl;

  tester->unitTest("PusB detailed deserialization check", []() {
    CCSDS::Packet pkt;
    pkt.setUpdatePacketEnable(true);
    auto& primary = pkt.getPrimaryHeader();
    primary.setAPID(0x123);
    primary.setDataFieldHeaderFlag(1);
    
    uint16_t eventId = 0xABCD;
    uint8_t svc = 17;
    uint8_t ssvc = 1;
    uint8_t srcId = 0x55;
    
    auto pusB = std::make_shared<PusB>(1, svc, ssvc, srcId, eventId, 0);
    pkt.setDataFieldHeader(pusB);
    TEST_VOID(pkt.setApplicationData({0xDE, 0xAD}));
    
    auto buffer = pkt.serialize();
    
    CCSDS::Packet pktDec;
    TEST_VOID(pktDec.deserialize(buffer, "PusB"));
    
    auto decSecondary = std::dynamic_pointer_cast<PusB>(pktDec.getDataField().getSecondaryHeader());
    if (!decSecondary) return false;
    
    if (decSecondary->getServiceType() != svc) return false;
    if (decSecondary->getServiceSubtype() != ssvc) return false;
    if (decSecondary->getSourceID() != srcId) return false;
    if (decSecondary->getEventID() != eventId) return false;
    if (decSecondary->getDataLength() != 2) return false;
    
    return true;
  });

  tester->unitTest("PusC variable length time code check", []() {
    CCSDS::Packet pkt;
    pkt.setUpdatePacketEnable(true);
    
    std::vector<uint8_t> timeCode = {0x11, 0x22, 0x33, 0x44, 0x55};
    auto pusC = std::make_shared<PusC>(1, 18, 1, 0x66, timeCode, 0);
    pkt.setDataFieldHeader(pusC);
    TEST_VOID(pkt.setApplicationData({0xAA, 0xBB}));
    
    auto buffer = pkt.serialize();
    
    CCSDS::Packet pktDec;
    // PusC size is 6 (fixed) + timeCode.size() = 11
    TEST_VOID(pktDec.deserialize(buffer, "PusC", 11));
    
    auto decSecondary = std::dynamic_pointer_cast<PusC>(pktDec.getDataField().getSecondaryHeader());
    if (!decSecondary) return false;
    
    if (decSecondary->getTimeCode() != timeCode) return false;
    if (decSecondary->getDataLength() != 2) return false;
    
    return true;
  });

  tester->unitTest("Large application data check (approx 1KB)", []() {
    CCSDS::Packet pkt;
    pkt.setUpdatePacketEnable(true);
    std::vector<uint8_t> largeData(1024, 0x5A);
    TEST_VOID(pkt.setApplicationData(largeData));
    
    auto buffer = pkt.serialize();
    if (buffer.size() < 1024 + 6 + 2) return false;
    
    CCSDS::Packet pktDec;
    TEST_VOID(pktDec.deserialize(buffer));
    
    auto decData = pktDec.getApplicationDataBytes();
    return decData == largeData;
  });

  tester->unitTest("Deserialization of too short data", []() {
    CCSDS::Packet pkt;
    std::vector<uint8_t> shortData = {0x00, 0x01, 0x02, 0x03, 0x04}; // Only 5 bytes
    auto res = pkt.deserialize(shortData);
    return !res; // Should fail
  });
}
