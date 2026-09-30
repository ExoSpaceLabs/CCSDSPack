// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/cuc.h"
#include "ccsdspack/c/bytes.h"

static uint64_t maximum_for_octets(const uint8_t octets) {
    if (octets == 0U) {
        return UINT64_C(0);
    }
    if (octets >= 8U) {
        return UINT64_MAX;
    }
    return (UINT64_C(1) << ((uint64_t)octets * UINT64_C(8))) - UINT64_C(1);
}

static uint8_t cuc_pfield(const ccsds_cuc_config_t *config) {
    const uint8_t epoch_bits =
        config->epoch == CCSDS_CUC_EPOCH_CCSDS_1958_TAI ? 0x10U : 0x20U;
    return (uint8_t)(epoch_bits
        | (uint8_t)((config->coarse_octets - 1U) << 2U)
        | config->fine_octets);
}

ccsds_status_t ccsds_cuc_validate(const ccsds_cuc_config_t *config) {
    if (config == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (config->epoch != CCSDS_CUC_EPOCH_CCSDS_1958_TAI
        && config->epoch != CCSDS_CUC_EPOCH_AGENCY_DEFINED) {
        return CCSDS_STATUS_CUC_INVALID_EPOCH;
    }
    if (config->pfield_mode != CCSDS_CUC_PFIELD_IMPLICIT
        && config->pfield_mode != CCSDS_CUC_PFIELD_EXPLICIT) {
        return CCSDS_STATUS_CUC_INVALID_PFIELD_MODE;
    }
    if (config->coarse_octets < 1U || config->coarse_octets > 4U) {
        return CCSDS_STATUS_CUC_INVALID_COARSE_WIDTH;
    }
    if (config->fine_octets > 3U) {
        return CCSDS_STATUS_CUC_INVALID_FINE_WIDTH;
    }
    return CCSDS_STATUS_OK;
}

size_t ccsds_cuc_encoded_size(const ccsds_cuc_config_t *config) {
    if (config == NULL) {
        return 0U;
    }
    return (size_t)config->coarse_octets
         + (size_t)config->fine_octets
         + (config->pfield_mode == CCSDS_CUC_PFIELD_EXPLICIT ? 1U : 0U);
}

ccsds_status_t ccsds_cuc_encode(const ccsds_cuc_time_t *value,
                                const ccsds_cuc_config_t *config,
                                uint8_t *output,
                                const size_t capacity,
                                size_t *written_out) {
    size_t offset = 0U;
    size_t required;
    ccsds_status_t status;

    if (value == NULL || config == NULL || written_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_cuc_validate(config);
    if (status != CCSDS_STATUS_OK) {
        return status;
    }
    if (value->coarse > maximum_for_octets(config->coarse_octets)) {
        return CCSDS_STATUS_CUC_COARSE_OVERFLOW;
    }
    if (value->fine > maximum_for_octets(config->fine_octets)) {
        return CCSDS_STATUS_CUC_FINE_OVERFLOW;
    }

    required = ccsds_cuc_encoded_size(config);
    if (capacity < required) {
        return CCSDS_STATUS_BUFFER_TOO_SMALL;
    }
    if (output == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    if (config->pfield_mode == CCSDS_CUC_PFIELD_EXPLICIT) {
        output[offset++] = cuc_pfield(config);
    }

    ccsds_store_be_uint(output + offset, config->coarse_octets, value->coarse);
    offset += (size_t)config->coarse_octets;
    if (config->fine_octets != 0U) {
        ccsds_store_be_uint(output + offset, config->fine_octets, value->fine);
        offset += (size_t)config->fine_octets;
    }

    *written_out = offset;
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_cuc_decode(const uint8_t *data,
                                const size_t size,
                                const ccsds_cuc_config_t *config,
                                ccsds_cuc_time_t *value_out) {
    size_t offset = 0U;
    const size_t expected = ccsds_cuc_encoded_size(config);
    ccsds_status_t status;

    if (config == NULL || value_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_cuc_validate(config);
    if (status != CCSDS_STATUS_OK) {
        return status;
    }
    if (size != expected) {
        return CCSDS_STATUS_CUC_SIZE_MISMATCH;
    }
    if (data == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    if (config->pfield_mode == CCSDS_CUC_PFIELD_EXPLICIT) {
        if (data[0] != cuc_pfield(config)) {
            return CCSDS_STATUS_CUC_PFIELD_MISMATCH;
        }
        offset = 1U;
    }

    value_out->coarse = ccsds_load_be_uint(data + offset, config->coarse_octets);
    offset += (size_t)config->coarse_octets;
    value_out->fine = config->fine_octets == 0U
        ? UINT64_C(0)
        : ccsds_load_be_uint(data + offset, config->fine_octets);

    return CCSDS_STATUS_OK;
}
