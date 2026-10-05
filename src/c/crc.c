// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/crc.h"

ccsds_status_t ccsds_crc16_update(uint16_t *state,
                                  const uint8_t *data,
                                  const size_t size,
                                  const uint16_t polynomial) {
    size_t index;
    uint16_t crc;

    if (state == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (data == NULL && size != 0U) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    crc = *state;
    for (index = 0U; index < size; ++index) {
        int bit;
        crc ^= (uint16_t)((uint16_t)data[index] << 8U);
        for (bit = 0; bit < 8; ++bit) {
            if ((crc & 0x8000U) != 0U) {
                crc = (uint16_t)((uint16_t)(crc << 1U) ^ polynomial);
            } else {
                crc = (uint16_t)(crc << 1U);
            }
        }
    }

    *state = crc;
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_crc16_compute(const uint8_t *data,
                                   const size_t size,
                                   const uint16_t polynomial,
                                   const uint16_t initial_value,
                                   const uint16_t final_xor_value,
                                   uint16_t *crc_out) {
    uint16_t state = initial_value;
    ccsds_status_t status;

    if (crc_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_crc16_update(&state, data, size, polynomial);
    if (status != CCSDS_STATUS_OK) {
        return status;
    }

    *crc_out = (uint16_t)(state ^ final_xor_value);
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_crc16_ccitt_false(const uint8_t *data,
                                       const size_t size,
                                       uint16_t *crc_out) {
    return ccsds_crc16_compute(data,
                              size,
                              CCSDS_CRC16_CCITT_FALSE_POLYNOMIAL,
                              CCSDS_CRC16_CCITT_FALSE_INITIAL,
                              CCSDS_CRC16_CCITT_FALSE_FINAL_XOR,
                              crc_out);
}
