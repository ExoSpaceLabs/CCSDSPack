// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/reassembly.h"
#include <string.h>

void ccsds_reassembly_reset(ccsds_reassembly_state_t *state,
                            const int validate_sequence) {
    if (state == NULL) {
        return;
    }
    ccsds_sequence_validator_reset(&state->sequence);
    state->written = 0U;
    state->validate_sequence = (uint8_t)(validate_sequence != 0);
}

ccsds_status_t ccsds_reassembly_accept(ccsds_reassembly_state_t *state,
                                       const ccsds_primary_header_t *header,
                                       const ccsds_buffer_view_t application_data,
                                       uint8_t *output,
                                       const size_t capacity,
                                       size_t *written_now_out,
                                       int *complete_out) {
    ccsds_sequence_validator_t staged_sequence;
    size_t available;
    int complete;
    ccsds_status_t status;

    if (state == NULL || header == NULL
        || written_now_out == NULL || complete_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (application_data.size != 0U && application_data.data == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (state->written > capacity) {
        return CCSDS_STATUS_INVALID_DATA;
    }
    available = capacity - state->written;
    if (application_data.size > available) {
        return CCSDS_STATUS_BUFFER_TOO_SMALL;
    }
    if (application_data.size != 0U && output == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    staged_sequence = state->sequence;
    if (state->validate_sequence != 0U) {
        if (!ccsds_sequence_flags_valid(&staged_sequence, header->sequence_flags)
            || !ccsds_sequence_count_valid(&staged_sequence, header->sequence_count)) {
            return CCSDS_STATUS_VALIDATION_FAILURE;
        }
        status = ccsds_sequence_validator_accept(&staged_sequence, header);
        if (status != CCSDS_STATUS_OK) {
            return status;
        }
    }

    complete = header->sequence_flags == 3U || header->sequence_flags == 2U;

    if (application_data.size != 0U) {
        memcpy(output + state->written,
               application_data.data,
               application_data.size);
    }

    state->sequence = staged_sequence;
    state->written += application_data.size;
    *written_now_out = application_data.size;
    *complete_out = complete;
    return CCSDS_STATUS_OK;
}
