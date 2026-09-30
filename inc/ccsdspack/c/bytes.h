// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_BYTES_H
#define CCSDSPACK_C_BYTES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Loads a big-endian 16-bit value. Caller provides at least two bytes. */
static inline uint16_t ccsds_load_be16(const uint8_t *data) {
    return (uint16_t)(((uint16_t)data[0] << 8U) | (uint16_t)data[1]);
}

/** @brief Stores a 16-bit value in big-endian order. Caller provides at least two bytes. */
static inline void ccsds_store_be16(uint8_t *data, const uint16_t value) {
    data[0] = (uint8_t)(value >> 8U);
    data[1] = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Loads 0..8 big-endian octets into a uint64_t.
 *
 * This is an unchecked primitive for codecs that have already validated the
 * pointer and width. A zero-octet field decodes to zero.
 */
static inline uint64_t ccsds_load_be_uint(const uint8_t *data, const uint8_t octets) {
    uint64_t value = 0U;
    uint8_t index;
    for (index = 0U; index < octets; ++index) {
        value = (value << 8U) | (uint64_t)data[index];
    }
    return value;
}

/**
 * @brief Stores the low-order 0..8 octets of a uint64_t in big-endian order.
 *
 * This is an unchecked primitive for codecs that have already validated the
 * pointer, width, and value range.
 */
static inline void ccsds_store_be_uint(uint8_t *data,
                                       const uint8_t octets,
                                       const uint64_t value) {
    uint8_t index;
    for (index = 0U; index < octets; ++index) {
        const uint8_t shift = (uint8_t)((octets - index - 1U) * 8U);
        data[index] = (uint8_t)((value >> shift) & UINT64_C(0xFF));
    }
}

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_BYTES_H
