// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/error.h"

const char *ccsds_status_name(const ccsds_status_t status) {
    switch (status) {
        case CCSDS_STATUS_OK: return "OK";
        case CCSDS_STATUS_UNKNOWN_ERROR: return "UNKNOWN_ERROR";
        case CCSDS_STATUS_NO_DATA: return "NO_DATA";
        case CCSDS_STATUS_INVALID_DATA: return "INVALID_DATA";
        case CCSDS_STATUS_INVALID_HEADER_DATA: return "INVALID_HEADER_DATA";
        case CCSDS_STATUS_INVALID_SECONDARY_HEADER_DATA: return "INVALID_SECONDARY_HEADER_DATA";
        case CCSDS_STATUS_INVALID_APPLICATION_DATA: return "INVALID_APPLICATION_DATA";
        case CCSDS_STATUS_NULL_POINTER: return "NULL_POINTER";
        case CCSDS_STATUS_INVALID_CHECKSUM: return "INVALID_CHECKSUM";
        case CCSDS_STATUS_VALIDATION_FAILURE: return "VALIDATION_FAILURE";
        case CCSDS_STATUS_TEMPLATE_SET_FAILURE: return "TEMPLATE_SET_FAILURE";
        case CCSDS_STATUS_FILE_READ_ERROR: return "FILE_READ_ERROR";
        case CCSDS_STATUS_FILE_WRITE_ERROR: return "FILE_WRITE_ERROR";
        case CCSDS_STATUS_CONFIG_FILE_ERROR: return "CONFIG_FILE_ERROR";
        case CCSDS_STATUS_BUFFER_TOO_SMALL: return "BUFFER_TOO_SMALL";
        default: return "UNRECOGNIZED_STATUS";
    }
}
