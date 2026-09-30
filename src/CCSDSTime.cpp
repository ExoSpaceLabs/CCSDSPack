// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "CCSDSTime.h"
#include "ccsdspack/c/cuc.h"

namespace {
  ccsds_cuc_config_t toCoreConfig(const ccsds::time::CucConfiguration &configuration) {
    return {
      static_cast<ccsds_cuc_epoch_t>(configuration.epoch),
      static_cast<ccsds_cuc_pfield_mode_t>(configuration.pField),
      configuration.coarseOctets,
      configuration.fineOctets
    };
  }

  ccsds::Error cucError(const ccsds_status_t status) {
    using ccsds::Error;
    using ccsds::ErrorCode;
    switch (status) {
      case CCSDS_STATUS_CUC_INVALID_EPOCH:
        return Error(ErrorCode::INVALID_DATA,
                     "CUC requires the CCSDS-1958 or an agency-defined epoch.");
      case CCSDS_STATUS_CUC_INVALID_PFIELD_MODE:
        return Error(ErrorCode::INVALID_DATA, "CUC P-field policy is invalid.");
      case CCSDS_STATUS_CUC_INVALID_COARSE_WIDTH:
        return Error(ErrorCode::INVALID_DATA,
                     "Basic CUC coarse time requires 1 to 4 octets.");
      case CCSDS_STATUS_CUC_INVALID_FINE_WIDTH:
        return Error(ErrorCode::INVALID_DATA,
                     "Basic CUC fine time requires 0 to 3 octets.");
      case CCSDS_STATUS_CUC_COARSE_OVERFLOW:
        return Error(ErrorCode::INVALID_DATA,
                     "CUC coarse time does not fit the configured width.");
      case CCSDS_STATUS_CUC_FINE_OVERFLOW:
        return Error(ErrorCode::INVALID_DATA,
                     "CUC fine time does not fit the configured width.");
      case CCSDS_STATUS_CUC_SIZE_MISMATCH:
        return Error(ErrorCode::INVALID_DATA,
                     "CUC encoded size does not match the configured layout.");
      case CCSDS_STATUS_CUC_PFIELD_MISMATCH:
        return Error(ErrorCode::INVALID_DATA,
                     "CUC P-field does not match the configured epoch or widths.");
      case CCSDS_STATUS_NULL_POINTER:
        return Error(ErrorCode::NULL_POINTER, "CUC core received a null pointer.");
      default:
        return Error(ErrorCode::INVALID_DATA, "CUC encoding or decoding failed.");
    }
  }
}

ccsds::ResultBool ccsds::time::validate(const CucConfiguration &configuration) {
  const auto core = toCoreConfig(configuration);
  const auto status = ccsds_cuc_validate(&core);
  if (status != CCSDS_STATUS_OK) return cucError(status);
  return true;
}

std::size_t ccsds::time::encodedSize(const CucConfiguration &configuration) {
  const auto core = toCoreConfig(configuration);
  return ccsds_cuc_encoded_size(&core);
}

ccsds::ResultBuffer ccsds::time::serialize(const CucTime &value,
                                           const CucConfiguration &configuration) {
  const auto coreConfig = toCoreConfig(configuration);
  const auto validation = ccsds_cuc_validate(&coreConfig);
  if (validation != CCSDS_STATUS_OK) return cucError(validation);

  const ccsds_cuc_time_t coreValue{value.coarse, value.fine};
  std::vector<std::uint8_t> output(ccsds_cuc_encoded_size(&coreConfig));
  std::size_t written = 0U;

  const auto status = ccsds_cuc_encode(
    &coreValue, &coreConfig, output.data(), output.size(), &written);
  if (status != CCSDS_STATUS_OK) return cucError(status);
  output.resize(written);
  return output;
}

ccsds::Result<ccsds::time::CucTime> ccsds::time::deserialize(
    const std::vector<std::uint8_t> &data,
    const CucConfiguration &configuration) {
  const auto coreConfig = toCoreConfig(configuration);
  ccsds_cuc_time_t value{};

  const auto status = ccsds_cuc_decode(data.data(), data.size(), &coreConfig, &value);
  if (status != CCSDS_STATUS_OK) return cucError(status);
  return CucTime{value.coarse, value.fine};
}
