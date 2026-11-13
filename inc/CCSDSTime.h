// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSTime.h
 * @brief Defines numeric CCSDS Unsegmented Time Code support.
 */
#ifndef CCSDS_TIME_H
#define CCSDS_TIME_H

#include <CCSDSResult.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ccsds::time {

  /** @brief Supported time-code families. */
  enum class Format : std::uint8_t {
    None = 0, ///< No time code.
    Cuc = 1   ///< CCSDS Unsegmented Time Code.
  };

  /** @brief Epoch metadata carried explicitly or by the mission profile. */
  enum class Epoch : std::uint8_t {
    Unspecified = 0,  ///< No epoch selected.
    Ccsds1958Tai = 1, ///< 1958-01-01 TAI CCSDS-recommended epoch.
    AgencyDefined = 2 ///< Epoch defined by the mission interface specification.
  };

  /** @brief Whether the CUC preamble field is encoded with the time field. */
  enum class PFieldMode : std::uint8_t {
    Implicit = 0, ///< P-field metadata is supplied by the mission profile.
    Explicit = 1  ///< A one-octet P-field precedes the CUC T-field.
  };

  /** @brief Mission-selected basic CUC layout from CCSDS 301.0-B-4. */
  struct CucConfiguration {
    Epoch epoch{Epoch::Unspecified};
    PFieldMode pField{PFieldMode::Implicit};
    std::uint8_t coarseOctets{0};
    std::uint8_t fineOctets{0};
  };

  /** @brief Numeric CUC counter relative to the configured epoch. */
  struct CucTime {
    std::uint64_t coarse{}; ///< Integral basic time units; seconds in this profile.
    std::uint64_t fine{};   ///< Binary fractional units scaled by 2^(8*fineOctets).

    [[nodiscard]] bool operator==(const CucTime &other) const {
      return coarse == other.coarse && fine == other.fine;
    }
    [[nodiscard]] bool operator!=(const CucTime &other) const { return !(*this == other); }
  };

  /** @brief Validates the supported basic CUC layout. */
  [[nodiscard]] ResultBool validate(const CucConfiguration &configuration);

  /** @brief Returns the encoded P-field plus T-field size. */
  [[nodiscard]] std::size_t encodedSize(const CucConfiguration &configuration);

  /** @brief Encodes a numeric CUC value in network byte order. */
  [[nodiscard]] ResultBuffer serialize(const CucTime &value,
                                       const CucConfiguration &configuration);

  /**
   * @brief Encodes basic numeric CUC directly into caller-owned storage.
   *
   * Delegates all wire, epoch, width, overflow and capacity checks to the
   * authoritative C11 codec. Allocates no heap memory. On failure, output
   * bytes are unchanged; the returned byte count is valid only on success.
   *
   * @see CCSDS 301.0-B-4, sections 3.2.1 and 3.2.2 (supported basic subset).
   */
  [[nodiscard]] Result<std::size_t> serialize(
    const CucTime &value, const CucConfiguration &configuration,
    std::uint8_t *output, std::size_t capacity);

  /**
   * @brief Decodes CUC from a borrowed exact-size byte span with no allocation.
   *
   * Uses the same C11 parser as the owning-vector overload. The caller owns
   * the input buffer and must retain its lifetime for the duration of the call.
   */
  [[nodiscard]] Result<CucTime> deserialize(
    const std::uint8_t *data, std::size_t size,
    const CucConfiguration &configuration);

  /** @brief Decodes one complete CUC value using the declared layout. */
  [[nodiscard]] Result<CucTime> deserialize(const std::vector<std::uint8_t> &data,
                                            const CucConfiguration &configuration);

} // namespace ccsds::time

#endif // CCSDS_TIME_H
