// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_BYTES_H
#define CCSDSPACK_C_BYTES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Explicit wire byte order for generic integer fields.
 *
 * CCSDS-defined primary headers and CUC fields remain big-endian by standard.
 * Mission/custom fields may select either order explicitly instead of depending
 * on the host CPU byte order.
 */
typedef enum ccsds_byte_order {
    CCSDS_BYTE_ORDER_BIG = 0,
    CCSDS_BYTE_ORDER_LITTLE = 1
} ccsds_byte_order_t;

/** @brief Returns non-zero when the byte-order selector is valid. */
static inline int ccsds_byte_order_is_valid(const ccsds_byte_order_t order) {
    return order == CCSDS_BYTE_ORDER_BIG || order == CCSDS_BYTE_ORDER_LITTLE;
}

/**
 * @brief Loads 0..8 octets into a uint64_t using explicit wire byte order.
 *
 * This primitive is independent of host endianness. The caller must provide a
 * valid pointer for non-zero widths and a valid byte-order selector.
 */
static inline uint64_t ccsds_load_uint(const uint8_t *data,
                                       const uint8_t octets,
                                       const ccsds_byte_order_t order) {
    uint64_t value = 0U;
    uint8_t index;

    if (order == CCSDS_BYTE_ORDER_BIG) {
        for (index = 0U; index < octets; ++index) {
            value = (value << 8U) | (uint64_t)data[index];
        }
    } else {
        for (index = 0U; index < octets; ++index) {
            value |= (uint64_t)data[index] << ((uint64_t)index * UINT64_C(8));
        }
    }

    return value;
}

/**
 * @brief Stores the low-order 0..8 octets using explicit wire byte order.
 *
 * This primitive is independent of host endianness. The caller must provide a
 * valid pointer for non-zero widths and a valid byte-order selector.
 */
static inline void ccsds_store_uint(uint8_t *data,
                                    const uint8_t octets,
                                    const uint64_t value,
                                    const ccsds_byte_order_t order) {
    uint8_t index;

    if (order == CCSDS_BYTE_ORDER_BIG) {
        for (index = 0U; index < octets; ++index) {
            const uint8_t shift = (uint8_t)((octets - index - 1U) * 8U);
            data[index] = (uint8_t)((value >> shift) & UINT64_C(0xFF));
        }
    } else {
        for (index = 0U; index < octets; ++index) {
            const uint8_t shift = (uint8_t)(index * 8U);
            data[index] = (uint8_t)((value >> shift) & UINT64_C(0xFF));
        }
    }
}

/** @brief Loads a 16-bit integer using explicit wire byte order. */
static inline uint16_t ccsds_load_u16(const uint8_t *data,
                                      const ccsds_byte_order_t order) {
    return (uint16_t)ccsds_load_uint(data, 2U, order);
}

/** @brief Stores a 16-bit integer using explicit wire byte order. */
static inline void ccsds_store_u16(uint8_t *data,
                                   const uint16_t value,
                                   const ccsds_byte_order_t order) {
    ccsds_store_uint(data, 2U, (uint64_t)value, order);
}

/** @brief Loads a CCSDS-standard big-endian 16-bit value. */
static inline uint16_t ccsds_load_be16(const uint8_t *data) {
    return ccsds_load_u16(data, CCSDS_BYTE_ORDER_BIG);
}

/** @brief Stores a CCSDS-standard big-endian 16-bit value. */
static inline void ccsds_store_be16(uint8_t *data, const uint16_t value) {
    ccsds_store_u16(data, value, CCSDS_BYTE_ORDER_BIG);
}

/** @brief Loads a little-endian 16-bit value for mission/custom fields. */
static inline uint16_t ccsds_load_le16(const uint8_t *data) {
    return ccsds_load_u16(data, CCSDS_BYTE_ORDER_LITTLE);
}

/** @brief Stores a little-endian 16-bit value for mission/custom fields. */
static inline void ccsds_store_le16(uint8_t *data, const uint16_t value) {
    ccsds_store_u16(data, value, CCSDS_BYTE_ORDER_LITTLE);
}

/** @brief Loads 0..8 CCSDS-standard big-endian octets. */
static inline uint64_t ccsds_load_be_uint(const uint8_t *data, const uint8_t octets) {
    return ccsds_load_uint(data, octets, CCSDS_BYTE_ORDER_BIG);
}

/** @brief Stores 0..8 octets in CCSDS-standard big-endian order. */
static inline void ccsds_store_be_uint(uint8_t *data,
                                       const uint8_t octets,
                                       const uint64_t value) {
    ccsds_store_uint(data, octets, value, CCSDS_BYTE_ORDER_BIG);
}

/** @brief Loads 0..8 little-endian octets for mission/custom fields. */
static inline uint64_t ccsds_load_le_uint(const uint8_t *data, const uint8_t octets) {
    return ccsds_load_uint(data, octets, CCSDS_BYTE_ORDER_LITTLE);
}

/** @brief Stores 0..8 octets in little-endian order for mission/custom fields. */
static inline void ccsds_store_le_uint(uint8_t *data,
                                       const uint8_t octets,
                                       const uint64_t value) {
    ccsds_store_uint(data, octets, value, CCSDS_BYTE_ORDER_LITTLE);
}

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_BYTES_H
