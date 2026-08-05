// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSTime.h"

namespace {
  std::uint64_t maximumForOctets(const std::uint8_t octets) {
    return octets >= 8U ? UINT64_MAX : ((std::uint64_t{1U} << (octets * 8U)) - 1U);
  }

  void appendCounter(std::vector<std::uint8_t> &output,
                     const std::uint64_t value,
                     const std::uint8_t octets) {
    for (std::uint8_t index = 0U; index < octets; ++index) {
      const auto shift = static_cast<std::uint8_t>((octets - index - 1U) * 8U);
      output.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
  }

  std::uint64_t readCounter(const std::vector<std::uint8_t> &data,
                            const std::size_t offset,
                            const std::uint8_t octets) {
    std::uint64_t value{};
    for (std::uint8_t index = 0U; index < octets; ++index) {
      value = (value << 8U) | data[offset + index];
    }
    return value;
  }

  std::uint8_t pField(const ccsds::time::CucConfiguration &configuration) {
    const std::uint8_t epoch = configuration.epoch == ccsds::time::Epoch::Ccsds1958Tai
                                 ? 0x10U : 0x20U;
    return static_cast<std::uint8_t>(epoch
      | ((configuration.coarseOctets - 1U) << 2U)
      | configuration.fineOctets);
  }
}

ccsds::ResultBool ccsds::time::validate(const CucConfiguration &configuration) {
  RET_IF_ERR_MSG(configuration.epoch != Epoch::Ccsds1958Tai
                 && configuration.epoch != Epoch::AgencyDefined,
                 ErrorCode::INVALID_DATA,
                 "CUC requires the CCSDS-1958 or an agency-defined epoch.");
  RET_IF_ERR_MSG(configuration.pField != PFieldMode::Implicit
                 && configuration.pField != PFieldMode::Explicit,
                 ErrorCode::INVALID_DATA, "CUC P-field policy is invalid.");
  RET_IF_ERR_MSG(configuration.coarseOctets < 1U || configuration.coarseOctets > 4U,
                 ErrorCode::INVALID_DATA,
                 "Basic CUC coarse time requires 1 to 4 octets.");
  RET_IF_ERR_MSG(configuration.fineOctets > 3U, ErrorCode::INVALID_DATA,
                 "Basic CUC fine time requires 0 to 3 octets.");
  return true;
}

std::size_t ccsds::time::encodedSize(const CucConfiguration &configuration) {
  return static_cast<std::size_t>(configuration.coarseOctets)
         + static_cast<std::size_t>(configuration.fineOctets)
         + (configuration.pField == PFieldMode::Explicit ? 1U : 0U);
}

ccsds::ResultBuffer ccsds::time::serialize(const CucTime &value,
                                           const CucConfiguration &configuration) {
  const auto validation = validate(configuration);
  if (!validation) return validation.error();
  RET_IF_ERR_MSG(value.coarse > maximumForOctets(configuration.coarseOctets),
                 ErrorCode::INVALID_DATA,
                 "CUC coarse time does not fit the configured width.");
  RET_IF_ERR_MSG(value.fine > maximumForOctets(configuration.fineOctets),
                 ErrorCode::INVALID_DATA,
                 "CUC fine time does not fit the configured width.");

  std::vector<std::uint8_t> output;
  output.reserve(encodedSize(configuration));
  if (configuration.pField == PFieldMode::Explicit) output.push_back(pField(configuration));
  appendCounter(output, value.coarse, configuration.coarseOctets);
  appendCounter(output, value.fine, configuration.fineOctets);
  return output;
}

ccsds::Result<ccsds::time::CucTime> ccsds::time::deserialize(
    const std::vector<std::uint8_t> &data,
    const CucConfiguration &configuration) {
  const auto validation = validate(configuration);
  if (!validation) return validation.error();
  RET_IF_ERR_MSG(data.size() != encodedSize(configuration), ErrorCode::INVALID_DATA,
                 "CUC encoded size does not match the configured layout.");

  std::size_t offset{};
  if (configuration.pField == PFieldMode::Explicit) {
    RET_IF_ERR_MSG(data.front() != pField(configuration), ErrorCode::INVALID_DATA,
                   "CUC P-field does not match the configured epoch or widths.");
    offset = 1U;
  }

  CucTime value;
  value.coarse = readCounter(data, offset, configuration.coarseOctets);
  value.fine = readCounter(data, offset + configuration.coarseOctets,
                           configuration.fineOctets);
  return value;
}
