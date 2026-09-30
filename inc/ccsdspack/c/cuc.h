// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_CUC_H
#define CCSDSPACK_C_CUC_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Epoch metadata for the supported basic numeric CUC profile. */
typedef enum ccsds_cuc_epoch {
    CCSDS_CUC_EPOCH_UNSPECIFIED = 0,
    CCSDS_CUC_EPOCH_CCSDS_1958_TAI = 1,
    CCSDS_CUC_EPOCH_AGENCY_DEFINED = 2
} ccsds_cuc_epoch_t;

/** @brief Whether the one-octet basic CUC P-field is carried on the wire. */
typedef enum ccsds_cuc_pfield_mode {
    CCSDS_CUC_PFIELD_IMPLICIT = 0,
    CCSDS_CUC_PFIELD_EXPLICIT = 1
} ccsds_cuc_pfield_mode_t;

/** @brief Supported CCSDS 301.0-B-4 basic numeric CUC layout. */
typedef struct ccsds_cuc_config {
    ccsds_cuc_epoch_t epoch;
    ccsds_cuc_pfield_mode_t pfield_mode;
    uint8_t coarse_octets;
    uint8_t fine_octets;
} ccsds_cuc_config_t;

/** @brief Numeric coarse/fine CUC counter relative to the configured epoch. */
typedef struct ccsds_cuc_time {
    uint64_t coarse;
    uint64_t fine;
} ccsds_cuc_time_t;

/** @brief Validates the supported basic CUC layout. */
ccsds_status_t ccsds_cuc_validate(const ccsds_cuc_config_t *config);

/**
 * @brief Returns the encoded P-field + T-field size without validating the layout.
 *
 * This mirrors the established C++ encodedSize() behavior. NULL returns zero.
 */
size_t ccsds_cuc_encoded_size(const ccsds_cuc_config_t *config);

/**
 * @brief Encodes one numeric CUC value directly into caller-owned storage.
 *
 * No allocation, ownership transfer, or hidden copy occurs.
 */
ccsds_status_t ccsds_cuc_encode(const ccsds_cuc_time_t *value,
                                const ccsds_cuc_config_t *config,
                                uint8_t *output,
                                size_t capacity,
                                size_t *written_out);

/**
 * @brief Decodes one complete numeric CUC value from caller-owned bytes.
 *
 * The input size must exactly match the configured layout.
 */
ccsds_status_t ccsds_cuc_decode(const uint8_t *data,
                                size_t size,
                                const ccsds_cuc_config_t *config,
                                ccsds_cuc_time_t *value_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_CUC_H
