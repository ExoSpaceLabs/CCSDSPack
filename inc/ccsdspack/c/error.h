// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_ERROR_H
#define CCSDSPACK_C_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Non-throwing status codes used by the C11 core.
 *
 * Values 0..13 intentionally align with the established CCSDSPack v2 C++
 * ErrorCode values so wrappers can preserve existing error semantics.
 */
typedef enum ccsds_status {
    CCSDS_STATUS_OK = 0,
    CCSDS_STATUS_UNKNOWN_ERROR = 1,
    CCSDS_STATUS_NO_DATA = 2,
    CCSDS_STATUS_INVALID_DATA = 3,
    CCSDS_STATUS_INVALID_HEADER_DATA = 4,
    CCSDS_STATUS_INVALID_SECONDARY_HEADER_DATA = 5,
    CCSDS_STATUS_INVALID_APPLICATION_DATA = 6,
    CCSDS_STATUS_NULL_POINTER = 7,
    CCSDS_STATUS_INVALID_CHECKSUM = 8,
    CCSDS_STATUS_VALIDATION_FAILURE = 9,
    CCSDS_STATUS_TEMPLATE_SET_FAILURE = 10,
    CCSDS_STATUS_FILE_READ_ERROR = 11,
    CCSDS_STATUS_FILE_WRITE_ERROR = 12,
    CCSDS_STATUS_CONFIG_FILE_ERROR = 13,
    CCSDS_STATUS_BUFFER_TOO_SMALL = 14
} ccsds_status_t;

/** @brief Returns a stable symbolic name for a C-core status value. */
const char *ccsds_status_name(ccsds_status_t status);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_ERROR_H
