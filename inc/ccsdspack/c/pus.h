// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_PUS_H
#define CCSDSPACK_C_PUS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Returns non-zero for supported mission identifier widths 0, 1, 2, or 4 octets. */
static inline int ccsds_pus_identifier_width_is_valid(const uint8_t octets) {
    return octets == 0U || octets == 1U || octets == 2U || octets == 4U;
}

/** @brief Returns non-zero when value fits exactly within the selected identifier width. */
static inline int ccsds_pus_identifier_fits(const uint32_t value, const uint8_t octets) {
    if (!ccsds_pus_identifier_width_is_valid(octets)) return 0;
    if (octets == 0U) return value == 0U;
    if (octets == 4U) return 1;
    return value < (UINT32_C(1) << ((uint32_t)octets * UINT32_C(8)));
}

/** @brief Returns non-zero when the configured trailing spare region contains only zero octets. */
static inline int ccsds_pus_spare_is_zero(const uint8_t *data,
                                          const size_t size,
                                          const uint8_t spare_octets) {
    size_t index;
    if (data == NULL || (size_t)spare_octets > size) return 0;
    for (index = size - (size_t)spare_octets; index < size; ++index) {
        if (data[index] != 0U) return 0;
    }
    return 1;
}

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_PUS_H
