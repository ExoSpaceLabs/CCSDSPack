// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_REASSEMBLY_H
#define CCSDSPACK_C_REASSEMBLY_H

#include <stddef.h>
#include <stdint.h>
#include "buffer.h"
#include "error.h"
#include "primary_header.h"
#include "validation.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocation-free receive-side application-data reassembly state.
 *
 * The output buffer is owned by the caller. written tracks the number of bytes
 * already committed to that buffer. When validate_sequence is non-zero, CCSDS
 * sequence flags/counts are checked before each append.
 */
typedef struct ccsds_reassembly_state {
    ccsds_sequence_validator_t sequence;
    size_t written;
    uint8_t validate_sequence;
} ccsds_reassembly_state_t;

/** @brief Initializes or resets one reassembly state. */
void ccsds_reassembly_reset(ccsds_reassembly_state_t *state,
                            int validate_sequence);

/**
 * @brief Appends one packet's application-data view into caller-owned output.
 *
 * header and application_data are non-owning inputs. output points to the base
 * of the caller-owned destination buffer. On any error no bytes are written and
 * the reassembly state is unchanged.
 *
 * complete_out reports whether this packet closes an application-data unit:
 * UNSEGMENTED and LAST are complete; FIRST and CONTINUING are not.
 */
ccsds_status_t ccsds_reassembly_accept(ccsds_reassembly_state_t *state,
                                       const ccsds_primary_header_t *header,
                                       ccsds_buffer_view_t application_data,
                                       uint8_t *output,
                                       size_t capacity,
                                       size_t *written_now_out,
                                       int *complete_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_REASSEMBLY_H
