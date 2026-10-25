// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPacket.h>
#include <iostream>
#include <memory>
#include <vector>
#include "tests.h"

namespace {
  template <typename T>
  bool hasErrorCode(const ccsds::Result<T> &result, const ccsds::ErrorCode code) {
    return !result && result.error().code() == code;
  }
}

void testGroupParsing(TestManager *tester, const std::string &description) {
  std::cout << "  testGroupParsing: " << description << std::endl;

  tester->unitTest("Bounded parsing consumes exactly one packet from a concatenated stream.", [] {
    ccsds::Packet first;
    ccsds::Packet second;
    TEST_VOID(first.setApplicationData({0x10, 0x20}));
    TEST_VOID(second.setApplicationData({0x30, 0x40, 0x50}));
    const auto firstBytes = serializedPacket(first);
    const auto secondBytes = serializedPacket(second);

    auto stream = firstBytes;
    stream.insert(stream.end(), secondBytes.begin(), secondBytes.end());

    ccsds::Packet decodedFirst;
    std::size_t consumed{};
    TEST_RET(consumed, decodedFirst.deserializeBounded(stream));
    if (consumed != firstBytes.size()
        || decodedFirst.getApplicationDataBytes() != std::vector<std::uint8_t>({0x10, 0x20})) {
      return false;
    }

    const std::vector<std::uint8_t> remainder(stream.begin() + static_cast<std::ptrdiff_t>(consumed), stream.end());
    ccsds::Packet decodedSecond;
    TEST_VOID(decodedSecond.deserialize(remainder));
    return decodedSecond.getApplicationDataBytes() == std::vector<std::uint8_t>({0x30, 0x40, 0x50});
  });

  tester->unitTest("Existing deserialize parses the first declared packet and ignores trailing bytes.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setApplicationData({0xAA, 0x55}));
    auto input = serializedPacket(source);
    input.insert(input.end(), {0xDE, 0xAD, 0xBE, 0xEF});

    ccsds::Packet decoded;
    TEST_VOID(decoded.deserialize(input));
    return decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0xAA, 0x55});
  });

  tester->unitTest("Truncated packet body returns a deterministic parsing error.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setApplicationData({1, 2, 3, 4}));
    auto input = serializedPacket(source);
    input.pop_back();

    ccsds::Packet decoded;
    const auto result = decoded.deserializeBounded(input);
    return hasErrorCode(result, ccsds::INVALID_DATA);
  });

  tester->unitTest("A declared Packet Data Length larger than the buffer is rejected.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setApplicationData({1, 2}));
    auto input = serializedPacket(source);
    input[4] = 0x00;
    input[5] = 0x20;

    ccsds::Packet decoded;
    const auto result = decoded.deserializeBounded(input);
    return hasErrorCode(result, ccsds::INVALID_DATA);
  });

  tester->unitTest("CRC corruption returns INVALID_CHECKSUM without mutating the target packet.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setApplicationData({0x10, 0x20, 0x30}));
    auto input = serializedPacket(source);
    input[6] ^= 0x01U;

    ccsds::Packet target;
    target.setUpdatePacketEnable(false);
    TEST_VOID(target.setApplicationData({0xAA}));
    const auto result = target.deserializeBounded(input);
    return hasErrorCode(result, ccsds::INVALID_CHECKSUM)
           && target.getApplicationDataBytes() == std::vector<std::uint8_t>({0xAA});
  });

  tester->unitTest("CRC-disabled bounded parsing skips checksum validation.", [] {
    ccsds::Packet source;
    source.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    TEST_VOID(source.setApplicationData({0x10, 0x20, 0x30}));
    auto input = serializedPacket(source);
    input[6] ^= 0x01U;

    ccsds::Packet decoded;
    decoded.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
    std::size_t consumed{};
    TEST_RET(consumed, decoded.deserializeBounded(input));
    return consumed == input.size()
           && decoded.getApplicationDataBytes() == std::vector<std::uint8_t>({0x11, 0x20, 0x30});
  });

  tester->unitTest("Unsupported packet versions are rejected during packet parsing.", [] {
    const std::vector<std::uint8_t> input{
      0x20U, 0x01U, 0xC0U, 0x00U, 0x00U, 0x00U, 0x01U
    };

    ccsds::Packet decoded;
    const auto result = decoded.deserializeBounded(input);
    return hasErrorCode(result, ccsds::INVALID_HEADER_DATA);
  });

  tester->unitTest("Every primary-header field rejects its first out-of-range value.", [] {
    ccsds::Header version;
    ccsds::Header type;
    ccsds::Header dataFieldFlag;
    ccsds::Header apid;
    ccsds::Header sequenceFlags;
    ccsds::Header sequenceCount;

    const auto versionResult = version.setVersionNumber(8);
    const auto typeResult = type.setType(2);
    const auto dataFieldResult = dataFieldFlag.setSecondaryHeaderFlag(2);
    const auto apidResult = apid.setAPID(2048);
    const auto flagsResult = sequenceFlags.setSequenceFlags(4);
    const auto countResult = sequenceCount.setSequenceCount(16384);

    return hasErrorCode(versionResult, ccsds::INVALID_HEADER_DATA)
           && hasErrorCode(typeResult, ccsds::INVALID_HEADER_DATA)
           && hasErrorCode(dataFieldResult, ccsds::INVALID_HEADER_DATA)
           && hasErrorCode(apidResult, ccsds::INVALID_HEADER_DATA)
           && hasErrorCode(flagsResult, ccsds::INVALID_HEADER_DATA)
           && hasErrorCode(countResult, ccsds::INVALID_HEADER_DATA)
           && version.serialize().empty()
           && type.serialize().empty()
           && dataFieldFlag.serialize().empty()
           && apid.serialize().empty()
           && sequenceFlags.serialize().empty()
           && sequenceCount.serialize().empty();
  });

  tester->unitTest("PrimaryHeader assignment is atomic when validation fails.", [] {
    ccsds::Header header;
    TEST_VOID(header.setData(ccsds::PrimaryHeader{0, 1, 0, 0x123, ccsds::FIRST_SEGMENT, 7, 9}));
    const auto result = header.setData(ccsds::PrimaryHeader{0, 1, 0, 0x123, 4, 7, 9});
    return hasErrorCode(result, ccsds::INVALID_HEADER_DATA)
           && header.getType() == 1
           && header.getAPID() == 0x123
           && header.getSequenceFlags() == ccsds::FIRST_SEGMENT
           && header.getSequenceCount() == 7
           && header.getDataLength() == 9;
  });

  tester->unitTest("Split-buffer CRC and no-CRC parsing preserve the C++ packet API.", [] {
    for (const auto mode : {ccsds::PacketErrorControlMode::CRC16,
                            ccsds::PacketErrorControlMode::None}) {
      ccsds::Packet source;
      source.setPacketErrorControlMode(mode);
      TEST_VOID(source.setApplicationData({0x10U, 0x20U, 0x30U}));
      const auto encoded = serializedPacket(source);
      const std::vector<std::uint8_t> header(encoded.begin(), encoded.begin() + 6);
      const std::vector<std::uint8_t> body(encoded.begin() + 6, encoded.end());

      ccsds::Packet split;
      split.setPacketErrorControlMode(mode);
      TEST_VOID(split.deserialize(header, body));
      ccsds::Packet contiguous;
      contiguous.setPacketErrorControlMode(mode);
      TEST_VOID(contiguous.deserialize(encoded));
      if (split.getApplicationDataBytes() != source.getApplicationDataBytes()
          || split.getApplicationDataBytes() != contiguous.getApplicationDataBytes()
          || split.getCRC() != contiguous.getCRC()) return false;
    }
    return true;
  });

  tester->unitTest("Split-buffer parser preserves CRC diagnostics and destination on failure.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setApplicationData({0x01U, 0x02U, 0x03U}));
    const auto wire = serializedPacket(source);
    const std::vector<std::uint8_t> header(wire.begin(), wire.begin() + 6);
    std::vector<std::uint8_t> body(wire.begin() + 6, wire.end());
    body[0] ^= 0x01U;

    ccsds::Packet target;
    target.setUpdatePacketEnable(false);
    TEST_VOID(target.setApplicationData({0xABU}));
    const auto result = target.deserialize(header, body);
    return hasErrorCode(result, ccsds::INVALID_CHECKSUM)
      && result.error().message() ==
           "Cannot deserialize packet: CRC16 packet error-control mismatch."
      && target.getApplicationDataBytes() == std::vector<std::uint8_t>({0xABU});
  });

  tester->unitTest("Split-buffer errors retain their established C++ codes and messages.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setApplicationData({0x11U, 0x22U}));
    const auto wire = serializedPacket(source);
    std::vector<std::uint8_t> header(wire.begin(), wire.begin() + 6);
    std::vector<std::uint8_t> body(wire.begin() + 6, wire.end());

    ccsds::Packet target;
    {
      auto shortHeader = header;
      shortHeader.pop_back();
      const auto result = target.deserialize(shortHeader, body);
      if (!hasErrorCode(result, ccsds::INVALID_HEADER_DATA)
          || result.error().message() !=
               "Cannot deserialize packet: primary header must contain exactly six bytes.")
        return false;
    }
    {
      auto shortBody = body;
      shortBody.pop_back();
      const auto result = target.deserialize(header, shortBody);
      if (!hasErrorCode(result, ccsds::INVALID_DATA)
          || result.error().message() !=
               "Cannot deserialize packet: packet body size does not match Packet Data Length.")
        return false;
    }
    {
      auto version1 = header;
      version1[0] |= 0x20U;
      const auto result = target.deserialize(version1, body);
      if (!hasErrorCode(result, ccsds::INVALID_HEADER_DATA)
          || result.error().message() !=
               "Cannot deserialize packet: unsupported CCSDS packet version.")
        return false;
    }
    {
      auto noCrcRoom = header;
      noCrcRoom[4] = 0U;
      noCrcRoom[5] = 0U; // declares a single body byte
      const auto result = target.deserialize(noCrcRoom, std::vector<std::uint8_t>{0x00U});
      if (!hasErrorCode(result, ccsds::INVALID_DATA)
          || result.error().message() !=
               "Cannot deserialize packet: CRC16 mode requires two packet error-control bytes.")
        return false;
    }
    return true;
  });

  tester->unitTest("Separated header and body input must match the declared packet length.", [] {
    ccsds::Packet source;
    TEST_VOID(source.setApplicationData({1, 2}));
    const auto encoded = serializedPacket(source);
    const std::vector<std::uint8_t> header(encoded.begin(), encoded.begin() + 6);
    std::vector<std::uint8_t> body(encoded.begin() + 6, encoded.end());
    body.pop_back();

    ccsds::Packet decoded;
    const auto result = decoded.deserialize(header, body);
    return hasErrorCode(result, ccsds::INVALID_DATA);
  });
}
