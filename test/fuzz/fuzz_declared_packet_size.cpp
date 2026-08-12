// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSBuffer.h>
#include <CCSDSHeader.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
  const auto declared = ccsds::buffer::declaredPacketSize(data, size);
  if (declared && (declared.value() < 7U || declared.value() > 65542U)) std::abort();

  if (size >= 6U) {
    const std::vector<std::uint8_t> headerBytes(data, data + 6U);
    ccsds::Header header;
    const auto parsed = header.deserialize(headerBytes);
    if (parsed && header.serialize().size() != 6U) std::abort();
  }
  return 0;
}
