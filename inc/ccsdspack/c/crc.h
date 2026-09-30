// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_CRC_H
#define CCSDSPACK_C_CRC_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CCSDS_CRC16_CCITT_FALSE_POLYNOMIAL ((uint16_t)0x1021U)
#define CCSDS_CRC16_CCITT_FALSE_INITIAL    ((uint16_t)0xFFFFU)
#define CCSDS_CRC16_CCITT_FALSE_FINAL_XOR  ((uint16_t)0x0000U)

/**
 * @brief Computes an MSB-first 16-bit CRC over caller-owned bytes.
 *
 * data may be NULL only when size is zero. No allocation or copying occurs.
 */
ccsds_status_t ccsds_crc16_compute(const uint8_t *data,
                                   size_t size,
                                   uint16_t polynomial,
                                   uint16_t initial_value,
                                   uint16_t final_xor_value,
                                   uint16_t *crc_out);

/** @brief Computes CRC-16/CCITT-FALSE using the CCSDSPack packet defaults. */
ccsds_status_t ccsds_crc16_ccitt_false(const uint8_t *data,
                                       size_t size,
                                       uint16_t *crc_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_CRC_H
