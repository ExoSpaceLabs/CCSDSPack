// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPack.h>

#include <cstdint>

int main() {
  ccsds::Packet packetTemplate;
  packetTemplate.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
  const auto headerResult = packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
    0, 0, 0, 0x155, ccsds::UNSEGMENTED, 0, 0
  });
  if (!headerResult) return headerResult.error().code();
  packetTemplate.setDataFieldSize(32U);

  ccsds::Manager sender;
  const auto templateResult = sender.setPacketTemplate(packetTemplate);
  if (!templateResult) return templateResult.error().code();
  sender.setAutoValidateEnable(false);

  const std::uint8_t payload[]{0xA0, 0xA1, 0xA2, 0xA3, 0xA4};
  const auto dataResult = sender.setApplicationData(payload, sizeof(payload));
  if (!dataResult) return dataResult.error().code();

  const ccsds::Manager &readOnlySender = sender;
  if (readOnlySender.getPacketsReference().size() != 1U) return 1;
  if (readOnlySender.getTemplateReference().getPrimaryHeader().getAPID() != 0x155U) return 2;
  (void)readOnlySender.getValidatorReference();

  const auto streamResult = sender.getPacketsBuffer();
  if (!streamResult) return streamResult.error().code();
  const auto &stream = streamResult.value();

  ccsds::Manager receiver;
  const auto receiverTemplate = receiver.setPacketTemplate(packetTemplate);
  if (!receiverTemplate) return receiverTemplate.error().code();
  receiver.setAutoValidateEnable(false);

  const auto loadResult = receiver.load(stream.data(), stream.size());
  if (!loadResult) return loadResult.error().code();

  const auto reconstructed = receiver.getApplicationDataBuffer();
  if (!reconstructed) return reconstructed.error().code();
  if (reconstructed.value().size() != sizeof(payload)) return 3;

  for (std::size_t i = 0; i < sizeof(payload); ++i) {
    if (reconstructed.value()[i] != payload[i]) return 4;
  }

  if (std::string(ccsds::errorCodeName(ccsds::ErrorCode::INVALID_DATA))
      != "INVALID_DATA") return 5;

  return 0;
}
