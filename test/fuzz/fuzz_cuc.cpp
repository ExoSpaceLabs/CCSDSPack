// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSTime.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
  if (size < 4U) return 0;

  ccsds::time::CucConfiguration configuration;
  configuration.epoch = static_cast<ccsds::time::Epoch>(data[0] % 4U);
  configuration.pField = static_cast<ccsds::time::PFieldMode>(data[1] % 3U);
  configuration.coarseOctets = static_cast<std::uint8_t>(data[2] % 6U);
  configuration.fineOctets = static_cast<std::uint8_t>(data[3] % 5U);

  const auto valid = ccsds::time::validate(configuration);
  if (!valid) return 0;

  const std::vector<std::uint8_t> encoded(data + 4U, data + size);
  const auto decoded = ccsds::time::deserialize(encoded, configuration);
  if (decoded) {
    const auto reencoded = ccsds::time::serialize(decoded.value(), configuration);
    if (!reencoded || reencoded.value() != encoded) std::abort();
  }
  return 0;
}
