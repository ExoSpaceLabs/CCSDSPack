// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPacket.h>
#include <iostream>
#include <memory>
#include <vector>
#include "PusServices.h"
#include "tests.h"

namespace {
  template <typename T>
  bool hasErrorCode(const CCSDS::Result<T> &result, const CCSDS::ErrorCode code) {
    return !result && result.error().code() == code;
  }
}

void testGroupParsing(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupParsing: " << description << std::endl;

  tester->unitTest("Bounded parsing consumes exactly one packet from a concatenated stream.", [] {
    CCSDS::Packet first;
    CCSDS::Packet second;
    TEST_VOID(first.setApplicationData({0x10, 0x20}));
    TEST_VOID(second.setApplicationData({0x30, 0x40, 0x50}));
    const auto firstBytes = first.serialize();
    const auto secondBytes = second.serialize();

    auto stream = firstBytes;
    stream.insert(stream.end(), secondBytes.begin(), secondBytes.end());

    CCSDS::Packet decodedFirst;
    std::size_t consumed{};
    TEST_RET(consumed, decodedFirst.deserializeBounded(stream));
    if (consumed != firstBytes.size()
        || decodedFirst.getApplicationDataBytes() != std::vector<std::uint8_t>({0x10, 0x20})) {
      return false;
    }

    const std::vector<std::uint8_t> remainder(
      stream.begin() + static_cast<std::ptrdiff_t>(consumed), stream.end());
    CCSDS::Packet decodedSecond;
    TEST_VOID(decodedSecond.deserialize(remainder));
    return decodedSecond.getApplicationDataBytes()
           == std::vector<std::uint8_t>({0x30, 0x40, 0x50});
  });

  tester->unitTest("Existing deserialize parses the first declared packet and ignores trailing bytes.", [] {
    CCSDS::Packet source;
    TEST_VOID(source.setApplicationData({0xAA, 0x55}));
    auto input = source.serialize();
    input.insert(input.end(), {0xDE, 0xAD, 0xBE, 0xEF});

    CCSDS::Packet decoded;
    TEST_VOID(decoded.deserialize(input));
    return decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0xAA, 0x55});
  });

  tester->unitTest("Truncated packet body returns a deterministic parsing error.", [] {
    CCSDS::Packet source;
    TEST_VOID(source.setApplicationData({1, 2, 3, 4}));
    auto input = source.serialize();
    input.pop_back();

    CCSDS::Packet decoded;
    const auto result = decoded.deserializeBounded(input);
    return hasErrorCode(result, CCSDS::INVALID_DATA);
  });

  tester->unitTest("A declared Packet Data Length larger than the buffer is rejected.", [] {
    CCSDS::Packet source;
    TEST_VOID(source.setApplicationData({1, 2}));
    auto input = source.serialize();
    input[4] = 0x00;
    input[5] = 0x20;

    CCSDS::Packet decoded;
    const auto result = decoded.deserializeBounded(input);
    return hasErrorCode(result, CCSDS::INVALID_DATA);
  });

  tester->unitTest("CRC corruption returns INVALID_CHECKSUM without mutating the target packet.", [] {
    CCSDS::Packet source;
    TEST_VOID(source.setApplicationData({0x10, 0x20, 0x30}));
    auto input = source.serialize();
    input[6] ^= 0x01U;

    CCSDS::Packet target;
    target.setUpdatePacketEnable(false);
    TEST_VOID(target.setApplicationData({0xAA}));
    const auto result = target.deserializeBounded(input);
    return hasErrorCode(result, CCSDS::INVALID_CHECKSUM)
           && target.getApplicationDataBytes() == std::vector<std::uint8_t>({0xAA});
  });

  tester->unitTest("CRC-disabled bounded parsing skips checksum validation.", [] {
    CCSDS::Packet source;
    source.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    TEST_VOID(source.setApplicationData({0x10, 0x20, 0x30}));
    auto input = source.serialize();
    input[6] ^= 0x01U;

    CCSDS::Packet decoded;
    decoded.setPacketErrorControlMode(CCSDS::PacketErrorControlMode::None);
    std::size_t consumed{};
    TEST_RET(consumed, decoded.deserializeBounded(input));
    return consumed == input.size()
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x11, 0x20, 0x30});
  });

  tester->unitTest("Typed secondary-header parsing respects the encoded packet boundary.", [] {
    CCSDS::Packet source;
    source.setDataFieldHeader(std::make_shared<PusA>(1, 2, 3, 4, 5));
    TEST_VOID(source.setApplicationData({0x60, 0x70}));
    const auto packetBytes = source.serialize();
    auto stream = packetBytes;
    stream.insert(stream.end(), {0x99, 0x88});

    CCSDS::Packet decoded;
    std::size_t consumed{};
    TEST_RET(consumed, decoded.deserializeBounded(stream, "PusA"));
    return consumed == packetBytes.size()
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x60, 0x70})
           && std::dynamic_pointer_cast<PusA>(decoded.getDataField().getSecondaryHeader()) != nullptr;
  });

  tester->unitTest("Unsupported packet versions are rejected during packet parsing.", [] {
    const std::vector<std::uint8_t> input{
      0x20, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00
    };

    CCSDS::Packet decoded;
    const auto result = decoded.deserializeBounded(input);
    return hasErrorCode(result, CCSDS::INVALID_HEADER_DATA);
  });

  tester->unitTest("Every primary-header field rejects its first invalid profile value.", [] {
    CCSDS::Header version;
    CCSDS::Header type;
    CCSDS::Header dataFieldFlag;
    CCSDS::Header apid;
    CCSDS::Header sequenceFlags;
    CCSDS::Header sequenceCount;

    const auto versionResult = version.setVersionNumber(1);
    const auto typeResult = type.setType(2);
    const auto dataFieldResult = dataFieldFlag.setDataFieldHeaderFlag(2);
    const auto apidResult = apid.setAPID(2048);
    const auto flagsResult = sequenceFlags.setSequenceFlags(4);
    const auto countResult = sequenceCount.setSequenceCount(16384);

    return hasErrorCode(versionResult, CCSDS::INVALID_HEADER_DATA)
           && hasErrorCode(typeResult, CCSDS::INVALID_HEADER_DATA)
           && hasErrorCode(dataFieldResult, CCSDS::INVALID_HEADER_DATA)
           && hasErrorCode(apidResult, CCSDS::INVALID_HEADER_DATA)
           && hasErrorCode(flagsResult, CCSDS::INVALID_HEADER_DATA)
           && hasErrorCode(countResult, CCSDS::INVALID_HEADER_DATA)
           && version.serialize().empty()
           && type.serialize().empty()
           && dataFieldFlag.serialize().empty()
           && apid.serialize().empty()
           && sequenceFlags.serialize().empty()
           && sequenceCount.serialize().empty();
  });

  tester->unitTest("PrimaryHeader assignment is atomic when validation fails.", [] {
    CCSDS::Header header;
    TEST_VOID(header.setData(CCSDS::PrimaryHeader{
      0, 1, 0, 0x123, CCSDS::FIRST_SEGMENT, 7, 9
    }));
    const auto result = header.setData(CCSDS::PrimaryHeader{
      0, 1, 0, 0x123, 4, 7, 9
    });
    return hasErrorCode(result, CCSDS::INVALID_HEADER_DATA)
           && header.getType() == 1
           && header.getAPID() == 0x123
           && header.getSequenceFlags() == CCSDS::FIRST_SEGMENT
           && header.getSequenceCount() == 7
           && header.getDataLength() == 9;
  });

  tester->unitTest("Separated header and body input must match the declared packet length.", [] {
    CCSDS::Packet source;
    TEST_VOID(source.setApplicationData({1, 2}));
    const auto encoded = source.serialize();
    const std::vector<std::uint8_t> header(encoded.begin(), encoded.begin() + 6);
    std::vector<std::uint8_t> body(encoded.begin() + 6, encoded.end());
    body.pop_back();

    CCSDS::Packet decoded;
    const auto result = decoded.deserialize(header, body);
    return hasErrorCode(result, CCSDS::INVALID_DATA);
  });
}
